#include "CString.h"
#include <cctype>
#include <algorithm>

//
// CStringUtil::CaselessCmp
//

bool
CStringUtil::CaselessCmp::cmpEqual(
				const CString::value_type& a,
				const CString::value_type& b)
{
	// FIXME -- use std::tolower but not in all versions of libstdc++
	return tolower(a) == tolower(b);
}

bool
CStringUtil::CaselessCmp::cmpLess(
				const CString::value_type& a,
				const CString::value_type& b)
{
	// FIXME -- use std::tolower but not in all versions of libstdc++
	return tolower(a) < tolower(b);
}

bool
CStringUtil::CaselessCmp::less(const CString& a, const CString& b)
{
	return std::lexicographical_compare(
								a.begin(), a.end(),
								b.begin(), b.end(),
								&CStringUtil::CaselessCmp::cmpLess);
}

bool
CStringUtil::CaselessCmp::equal(const CString& a, const CString& b)
{
	return !(less(a, b) || less(b, a));
}

bool
CStringUtil::CaselessCmp::operator()(const CString& a, const CString& b) const
{
	return less(a, b);
}
