#!/usr/bin/env python3

# Synergy -- mouse and keyboard sharing utility
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

import lib.env as env

env.ensure_in_venv(__file__)

import argparse
import sys
import re
import lib.github as github

ISSUE_BODY_ENV = "ISSUE_BODY"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--issue-check-tech-support",
        action="store_true",
        help="Check if an issue may need tech support",
    )
    args = parser.parse_args()

    if args.issue_check_tech_support:
        issue_check_tech_support()
    else:
        print("No command specified", file=sys.stderr)
        sys.exit(1)


def issue_check_tech_support():
    print("Checking if issue may need tech support")

    issue_body = env.get_env(ISSUE_BODY_ENV)
    print(f'Issue body...\n"{issue_body}"\n')

    customer_pattern = r"What type of Synergy user are you\?\s*Customer"
    missing_ticket_pattern = r"Support ticket number \(for customers\)\s*_No response_"

    is_customer = re.search(customer_pattern, issue_body, re.DOTALL)
    no_ticket = re.search(missing_ticket_pattern, issue_body, re.DOTALL)

    print(f"Is customer: {is_customer}")
    print(f"No ticket: {no_ticket}")

    may_need_tech_support = bool(is_customer and no_ticket)
    github.set_output("may-need-tech-support", str(may_need_tech_support).lower())


if __name__ == "__main__":
    main()
