import colorama  # type: ignore
from colorama import Fore  # type: ignore

colorama.init()

SUCCESS_TEXT = f"{Fore.LIGHTGREEN_EX}Success:{Fore.RESET}"
ERROR_TEXT = f"{Fore.RED}Error:{Fore.RESET}"
WARNING_TEXT = f"{Fore.LIGHTYELLOW_EX}Warning:{Fore.RESET}"
HINT_TEXT = f"{Fore.LIGHTBLUE_EX}Hint:{Fore.RESET}"
