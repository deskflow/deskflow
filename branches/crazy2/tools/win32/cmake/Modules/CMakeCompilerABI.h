/*--------------------------------------------------------------------------*/

/* Size of a pointer-to-data in bytes.  */
#define SIZEOF_DPTR (sizeof(void*))
const char info_sizeof_dptr[] =  {
  'I', 'N', 'F', 'O', ':', 's', 'i', 'z', 'e', 'o', 'f', '_', 'd', 'p', 't', 'r', '[',
  ('0' + ((SIZEOF_DPTR / 10)%10)),
  ('0' +  (SIZEOF_DPTR    % 10)),
  ']','\0'};

/*--------------------------------------------------------------------------*/

/* Application Binary Interface.  */
#if defined(__sgi) && defined(_ABIO32)
# define ABI_ID "ELF O32"
#elif defined(__sgi) && defined(_ABIN32)
# define ABI_ID "ELF N32"
#elif defined(__sgi) && defined(_ABI64)
# define ABI_ID "ELF 64"
#elif defined(__ELF__)
# define ABI_ID "ELF"
#endif

#if defined(ABI_ID)
static char const info_abi[] = "INFO:abi[" ABI_ID "]";
#endif
