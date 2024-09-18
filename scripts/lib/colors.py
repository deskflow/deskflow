# Deskflow -- mouse and keyboard sharing utility
# Copyright (C) 2024 Symless Ltd.
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import colorama  # type: ignore
from colorama import Fore  # type: ignore

colorama.init()

SUCCESS_TEXT = f"{Fore.LIGHTGREEN_EX}Success:{Fore.RESET}"
ERROR_TEXT = f"{Fore.LIGHTRED_EX}Error:{Fore.RESET}"
WARNING_TEXT = f"{Fore.LIGHTYELLOW_EX}Warning:{Fore.RESET}"
HINT_TEXT = f"{Fore.LIGHTBLUE_EX}Hint:{Fore.RESET}"
