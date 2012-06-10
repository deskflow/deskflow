/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#include <rfb/HTTPServer.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>
#include <rdr/MemOutStream.h>

#ifdef WIN32
#define strcasecmp _stricmp
#endif

#pragma warning(disable: 4244)

using namespace rfb;
using namespace rdr;

static LogWriter vlog("HTTPServer");

const int clientWaitTimeMillis = 20000;
const int idleTimeoutSecs = 5 * 60;


//
// -=- LineReader
//     Helper class which is repeatedly called until a line has been read
//     (lines end in \n or \r\n).
//     Returns true when line complete, and resets internal state so that
//     next read() call will start reading a new line.
//     Only one buffer is kept - process line before reading next line!
//

class LineReader : public CharArray {
public:
  LineReader(InStream& is_, int l)
    : CharArray(l), is(is_), pos(0), len(l), bufferOverrun(false) {}

  // Returns true if line complete, false otherwise
  bool read() {
    while (is.checkNoWait(1)) {
      char c = is.readU8();

      if (c == '\n') {
        if (pos && (buf[pos-1] == '\r'))
          pos--;
        bufferOverrun = false;
        buf[pos++] = 0;
        pos = 0;
        return true;
      }

      if (pos == (len-1)) {
        bufferOverrun = true;
        buf[pos] = 0;
        return true;
      }

      buf[pos++] = c;
    }

    return false;
  }
  bool didBufferOverrun() const {return bufferOverrun;}
protected:
  InStream& is;
  int pos, len;
  bool bufferOverrun;
};


//
// -=- HTTPServer::Session
//     Manages the internal state for an HTTP session.
//     processHTTP returns true when request has completed,
//     indicating that socket & session data can be deleted.
//

class rfb::HTTPServer::Session {
public:
  Session(network::Socket& s, rfb::HTTPServer& srv)
    : contentType(0), contentLength(-1), lastModified(-1),
      line(s.inStream(), 256), sock(s),
      server(srv), state(ReadRequestLine), lastActive(time(0)) {
  }
  ~Session() {
  }

  void writeResponse(int result, const char* text);
  bool writeResponse(int code);

  bool processHTTP();

  network::Socket* getSock() const {return &sock;}

  int checkIdleTimeout();
protected:
  CharArray uri;
  const char* contentType;
  int contentLength;
  time_t lastModified;
  LineReader line;
  network::Socket& sock;
  rfb::HTTPServer& server;
  enum {ReadRequestLine, ReadHeaders, WriteResponse} state;
  enum {GetRequest, HeadRequest} request;
  time_t lastActive;
};


// - Internal helper routines

void
copyStream(InStream& is, OutStream& os) {
  try {
    while (1) {
      os.writeU8(is.readU8());
    }
  } catch (rdr::EndOfStream) {
  }
}

void writeLine(OutStream& os, const char* text) {
  os.writeBytes(text, strlen(text));
  os.writeBytes("\r\n", 2);
}


// - Write an HTTP-compliant response to the client


void
HTTPServer::Session::writeResponse(int result, const char* text) {
  char buffer[1024];
  if (strlen(text) > 512)
    throw new rdr::Exception("Internal error - HTTP response text too big");
  sprintf(buffer, "%s %d %s", "HTTP/1.1", result, text);
  OutStream& os=sock.outStream();
  writeLine(os, buffer);
  writeLine(os, "Server: RealVNC/4.0");
  time_t now = time(0);
  struct tm* tm = gmtime(&now);
  strftime(buffer, 1024, "Date: %a, %d %b %Y %H:%M:%S GMT", tm);
  writeLine(os, buffer);
  if (lastModified == (time_t)-1 || lastModified == 0)
    lastModified = now;
  tm = gmtime(&lastModified);
  strftime(buffer, 1024, "Last-Modified: %a, %d %b %Y %H:%M:%S GMT", tm);
  writeLine(os, buffer);
  if (contentLength != -1) {
    sprintf(buffer,"Content-Length: %d",contentLength);
    writeLine(os, buffer);
  }
  writeLine(os, "Connection: close");
  os.writeBytes("Content-Type: ", 14);
  if (result == 200) {
    if (!contentType)
      contentType = guessContentType(uri.buf, "text/html");
    os.writeBytes(contentType, strlen(contentType));
  } else {
    os.writeBytes("text/html", 9);
  }
  os.writeBytes("\r\n", 2);
  writeLine(os, "");
  if (result != 200) {
    writeLine(os, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">");
    writeLine(os, "<HTML><HEAD>");
    sprintf(buffer, "<TITLE>%d %s</TITLE>", result, text);
    writeLine(os, buffer);
    writeLine(os, "</HEAD><BODY><H1>");
    writeLine(os, text);
    writeLine(os, "</H1></BODY></HTML>");
    sock.outStream().flush();
  }
}

bool
HTTPServer::Session::writeResponse(int code) {
  switch (code) {
  case 200: writeResponse(code, "OK"); break;
  case 400: writeResponse(code, "Bad Request"); break;
  case 404: writeResponse(code, "Not Found"); break;
  case 501: writeResponse(code, "Not Implemented"); break;
  default: writeResponse(500, "Unknown Error"); break;
  };

  // This return code is passed straight out of processHTTP().
  // true indicates that the request has been completely processed.
  return true;
}

// - Main HTTP request processing routine

bool
HTTPServer::Session::processHTTP() {
  lastActive = time(0);

  while (sock.inStream().checkNoWait(1)) {

    switch (state) {

      // Reading the Request-Line
    case ReadRequestLine:

      // Either read a line, or run out of incoming data
      if (!line.read())
        return false;

      // We have read a line!  Skip it if it's blank
      if (strlen(line.buf) == 0)
        continue;

      // The line contains a request to process.
      {
        char method[16], path[128], version[16];
        int matched = sscanf(line.buf, "%15s%127s%15s",
          method, path, version);
        if (matched != 3)
          return writeResponse(400);

        // Store the required "method"
        if (strcmp(method, "GET") == 0)
          request = GetRequest;
        else if (strcmp(method, "HEAD") == 0)
          request = HeadRequest;
        else
          return writeResponse(501);

        // Store the URI to the "document"
        uri.buf = strDup(path);
      }

      // Move on to reading the request headers
      state = ReadHeaders;
      break;

      // Reading the request headers
    case ReadHeaders:

      // Try to read a line
      if (!line.read())
        return false;

      // Skip headers until we hit a blank line
      if (strlen(line.buf) != 0)
        continue;

      // Headers ended - write the response!
      {
        CharArray address(sock.getPeerAddress());
        vlog.info("getting %s for %s", uri.buf, address.buf);
        contentLength = -1;
        lastModified = -1;
        InStream* data = server.getFile(uri.buf, &contentType, &contentLength,
                                        &lastModified);
        if (!data)
          return writeResponse(404);

        try {
          writeResponse(200);
          if (request == GetRequest)
            copyStream(*data, sock.outStream());
          sock.outStream().flush();
        } catch (rdr::Exception& e) {
          vlog.error("error writing HTTP document:%s", e.str());
        }
        delete data;
      }

      // The operation is complete!
      return true;

    default:
      throw rdr::Exception("invalid HTTPSession state!");
    };

  }

  // Indicate that we're still processing the HTTP request.
  return false;
}

int HTTPServer::Session::checkIdleTimeout() {
  time_t now = time(0);
  int timeout = (lastActive + idleTimeoutSecs) - now;
  if (timeout > 0)
    return secsToMillis(timeout);
  sock.shutdown();
  return 0;
}

// -=- Constructor / destructor

HTTPServer::HTTPServer() {
}

HTTPServer::~HTTPServer() {
  std::list<Session*>::iterator i;
  for (i=sessions.begin(); i!=sessions.end(); i++)
    delete *i;
}


// -=- SocketServer interface implementation

void
HTTPServer::addSocket(network::Socket* sock, bool) {
  Session* s = new Session(*sock, *this);
  if (!s) {
    sock->shutdown();
  } else {
    sock->inStream().setTimeout(clientWaitTimeMillis);
    sock->outStream().setTimeout(clientWaitTimeMillis);
    sessions.push_front(s);
  }
}

void
HTTPServer::removeSocket(network::Socket* sock) {
  std::list<Session*>::iterator i;
  for (i=sessions.begin(); i!=sessions.end(); i++) {
    if ((*i)->getSock() == sock) {
      delete *i;
      sessions.erase(i);
      return;
    }
  }
}

void
HTTPServer::processSocketEvent(network::Socket* sock) {
  std::list<Session*>::iterator i;
  for (i=sessions.begin(); i!=sessions.end(); i++) {
    if ((*i)->getSock() == sock) {
      try {
        if ((*i)->processHTTP()) {
          vlog.info("completed HTTP request");
          sock->shutdown();
        }
      } catch (rdr::Exception& e) {
        vlog.error("untrapped: %s", e.str());
        sock->shutdown();
      }
      return;
    }
  }
  throw rdr::Exception("invalid Socket in HTTPServer");
}

void HTTPServer::getSockets(std::list<network::Socket*>* sockets)
{
  sockets->clear();
  std::list<Session*>::iterator ci;
  for (ci = sessions.begin(); ci != sessions.end(); ci++) {
    sockets->push_back((*ci)->getSock());
  }
}

int HTTPServer::checkTimeouts() {
  std::list<Session*>::iterator ci;
  int timeout = 0;
  for (ci = sessions.begin(); ci != sessions.end(); ci++) {
    soonestTimeout(&timeout, (*ci)->checkIdleTimeout());
  }
  return timeout;
}


// -=- Default getFile implementation

InStream*
HTTPServer::getFile(const char* name, const char** contentType,
                    int* contentLength, time_t* lastModified)
{
  return 0;
}

const char*
HTTPServer::guessContentType(const char* name, const char* defType) {
  CharArray file, ext;
  if (!strSplit(name, '.', &file.buf, &ext.buf))
    return defType;
  if (strcasecmp(ext.buf, "html") == 0 ||
    strcasecmp(ext.buf, "htm") == 0) {
    return "text/html";
  } else if (strcasecmp(ext.buf, "txt") == 0) {
    return "text/plain";
  } else if (strcasecmp(ext.buf, "gif") == 0) {
    return "image/gif";
  } else if (strcasecmp(ext.buf, "jpg") == 0) {
    return "image/jpeg";
  } else if (strcasecmp(ext.buf, "jar") == 0) {
    return "application/java-archive";
  } else if (strcasecmp(ext.buf, "exe") == 0) {
    return "application/octet-stream";
  }
  return defType;
}
