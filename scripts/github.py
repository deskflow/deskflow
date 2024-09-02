#!/usr/bin/env python3

import lib.env as env

env.ensure_in_venv(__file__)

import requests
import argparse
from dotenv import load_dotenv  # type: ignore

ENV_FILE = ".env"
GITHUB_API = "https://api.github.com"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--delete-release-matches",
        type=str,
        help="String to match for releases to delete",
    )
    parser.add_argument(
        "--delete-release-drafts", action="store_true", help="Delete draft releases"
    )
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    load_dotenv(dotenv_path=ENV_FILE)

    if args.delete_release_matches:
        delete_release_matches(args.delete_release_matches, args.dry_run)
    elif args.delete_release_drafts:
        delete_release_drafts(args.dry_run)
    else:
        raise RuntimeError("No action specified")


def delete_release_matches(match: str, dry_run: bool):
    print("Deleting releases matching:", match)

    GITHUB_REPO = env.get_env("GITHUB_REPO")
    GITHUB_TOKEN = env.get_env("GITHUB_TOKEN")

    releases_url = f"{GITHUB_API}/repos/{GITHUB_REPO}/releases"
    headers = {"Authorization": f"token {GITHUB_TOKEN}"}

    page = 1
    per_page = 100
    max_pages = 10

    while page <= max_pages:
        if delete_release_matches_page(
            releases_url, headers, page, per_page, match, dry_run
        ):
            page += 1
        else:
            break

    print("Finished deleting releases")


def delete_release_matches_page(
    releases_url: str,
    headers: dict,
    page: int,
    per_page: int,
    match: str,
    dry_run: bool,
):
    print(f"Fetching page {page} of releases")

    params = {"page": page, "per_page": per_page}
    response = requests.get(releases_url, headers=headers, params=params)
    response.raise_for_status()

    releases = response.json()
    if not releases:
        return

    for release in releases:
        if match in release["name"]:
            delete_release(releases_url, release, headers, dry_run)

    # Return true if we fetched the maximum number of releases per page,
    # which means there might be more releases to fetch.
    return len(releases) == per_page


def delete_release_drafts(dry_run: bool):
    print("Deleting draft releases")

    GITHUB_REPO = env.get_env("GITHUB_REPO")
    GITHUB_TOKEN = env.get_env("GITHUB_TOKEN")

    releases_url = f"{GITHUB_API}/repos/{GITHUB_REPO}/releases"
    headers = {"Authorization": f"token {GITHUB_TOKEN}"}

    page = 1
    per_page = 100
    max_pages = 10

    while page <= max_pages:
        if delete_release_drafts_page(releases_url, headers, page, per_page, dry_run):
            page += 1
        else:
            break

    print("Finished deleting draft releases")


def delete_release_drafts_page(
    releases_url: str, headers: dict, page: int, per_page: int, dry_run: bool
):
    print(f"Fetching page {page} of releases")

    params = {"page": page, "per_page": per_page}
    response = requests.get(releases_url, headers=headers, params=params)
    response.raise_for_status()

    releases = response.json()
    if not releases:
        return

    for release in releases:
        if release["draft"]:
            delete_release(releases_url, release, headers, dry_run)

    # Return true if we fetched the maximum number of releases per page,
    # which means there might be more releases to fetch.
    return len(releases) == per_page


def delete_release(releases_url: str, release: dict, headers: dict, dry_run: bool):
    release_id = release["id"]
    tag_name = release["tag_name"]

    if dry_run:
        print(f"Would delete release: {tag_name}")
        return
    else:
        print(f"Deleting release: {tag_name}")

    release_url = f"{releases_url}/{release_id}"
    response = requests.delete(release_url, headers=headers)
    response.raise_for_status()


if __name__ == "__main__":
    main()
