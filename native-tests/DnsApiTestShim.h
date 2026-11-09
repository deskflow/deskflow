// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// TEST ONLY: portable semantic fixture for documented windns.h members.
// This is not the Windows SDK and cannot validate its ABI or OS implementation.
#pragma once
#include <arpa/inet.h>
#include <cstdint>
#define WINAPI
using VOID = void;
using PVOID = void *;
using DWORD = std::uint32_t;
using ULONG = std::uint32_t;
using WORD = std::uint16_t;
using BOOL = int;
using PCWSTR = const wchar_t *;
using PWSTR = wchar_t *;
using IP4_ADDRESS = std::uint32_t;
struct IP6_ADDRESS
{
  std::uint8_t bytes[16];
};
constexpr DWORD ERROR_SUCCESS = 0, ERROR_CANCELLED = 1223, DNS_REQUEST_PENDING = 9506, DNS_QUERY_REQUEST_VERSION1 = 1;
constexpr int FALSE = 0, DNS_TYPE_PTR = 12, DnsFreeRecordList = 1;
struct DNS_RECORD
{
  DNS_RECORD *pNext = nullptr;
  WORD wType = 0;
  DWORD dwTtl = 0;
  struct
  {
    struct
    {
      PWSTR pNameHost = nullptr;
    } PTR;
  } Data;
};
using PDNS_RECORD = DNS_RECORD *;
struct DNS_SERVICE_INSTANCE
{
  PWSTR pszInstanceName = nullptr, pszHostName = nullptr;
  IP4_ADDRESS *ip4Address = nullptr;
  IP6_ADDRESS *ip6Address = nullptr;
  WORD wPort = 0, wPriority = 0, wWeight = 0;
  DWORD dwPropertyCount = 0;
  PWSTR *keys = nullptr;
  PWSTR *values = nullptr;
  DWORD dwInterfaceIndex = 0;
};
using PDNS_SERVICE_INSTANCE = DNS_SERVICE_INSTANCE *;
using BrowseCallback = VOID(WINAPI *)(DWORD, PVOID, PDNS_RECORD);
using InstanceCallback = VOID(WINAPI *)(DWORD, PVOID, PDNS_SERVICE_INSTANCE);
struct DNS_SERVICE_BROWSE_REQUEST
{
  ULONG Version = 0, InterfaceIndex = 0;
  PCWSTR QueryName = nullptr;
  BrowseCallback pBrowseCallback = nullptr;
  PVOID pQueryContext = nullptr;
};
struct DNS_SERVICE_RESOLVE_REQUEST
{
  ULONG Version = 0, InterfaceIndex = 0;
  PWSTR QueryName = nullptr;
  InstanceCallback pResolveCompletionCallback = nullptr;
  PVOID pQueryContext = nullptr;
};
struct DNS_SERVICE_REGISTER_REQUEST
{
  ULONG Version = 0, InterfaceIndex = 0;
  PDNS_SERVICE_INSTANCE pServiceInstance = nullptr;
  InstanceCallback pRegisterCompletionCallback = nullptr;
  PVOID pQueryContext = nullptr;
  PVOID hCredentials = nullptr;
  BOOL unicastEnabled = 0;
};
struct DNS_SERVICE_CANCEL
{
  PVOID reserved = nullptr;
};
DWORD DnsServiceBrowse(DNS_SERVICE_BROWSE_REQUEST *, DNS_SERVICE_CANCEL *);
DWORD DnsServiceBrowseCancel(DNS_SERVICE_CANCEL *);
DWORD DnsServiceResolve(DNS_SERVICE_RESOLVE_REQUEST *, DNS_SERVICE_CANCEL *);
DWORD DnsServiceResolveCancel(DNS_SERVICE_CANCEL *);
DWORD DnsServiceRegister(DNS_SERVICE_REGISTER_REQUEST *, DNS_SERVICE_CANCEL *);
DWORD DnsServiceDeRegister(DNS_SERVICE_REGISTER_REQUEST *, DNS_SERVICE_CANCEL *);
PDNS_SERVICE_INSTANCE
DnsServiceConstructInstance(PCWSTR, PCWSTR, IP4_ADDRESS *, IP6_ADDRESS *, WORD, WORD, WORD, DWORD, PCWSTR *, PCWSTR *);
void DnsServiceFreeInstance(PDNS_SERVICE_INSTANCE);
void DnsRecordListFree(PDNS_RECORD, int);
