#ifndef LOGINRESULT_H
#define LOGINRESULT_H

enum qLoginResult {
	Unknown,
	Student,
	Home,
	Professional,
	Error,
	ExceptionError,
	InvalidEmailPassword,
	ServerResponseError
};

#endif // LOGINRESULT_H
