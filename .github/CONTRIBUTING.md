<!---
  SPDX-FileCopyrightText: 2023 SAP SE

  SPDX-License-Identifier: Apache-2.0

  This file is part of FEDEM - https://openfedem.org
--->

# Contributing

## Code of Conduct

All members of the project community must abide by the
[Contributor's Coventant 2.1 Code of Conduct](https://www.contributor-covenant.org/version/2/1/code_of_conduct/).
Only by respecting each other we can develop a productive, collaborative community.
Instances of abusive, harassing, or otherwise unacceptable behavior may be reported by contacting [a project maintainer](../.reuse/dep5).

## Engaging in Our Project

We use GitHub to manage reviews of pull requests.

* If you are a new contributor, see [Steps to Contribute](#steps-to-contribute).

* Before implementing your change, create an issue that describes the problem you would like to solve or the code that should be enhanced. Please note that you are willing to work on that issue.

* The team will review the issue and decide whether it should be implemented as a pull request. In that case, they will assign the issue to you. If the team decides against picking up the issue, the team will post a comment with an explanation.

## Steps to Contribute

Should you wish to work on an issue, please claim it first by commenting on the GitHub issue that you want to work on. This is to prevent duplicated efforts from other contributors on the same issue.

If you have questions about one of the issues, please comment on them, and one of the maintainers will clarify.

## Contributing Code or Documentation

You are welcome to contribute code in order to fix a bug or to implement a new feature that is logged as an issue.

All code contributions must be licensed under the [Apache 2.0 License](../LICENSE).

## Automated Version Bumps

The [versioning workflow](workflows/versioning.yml) increments `cfg/VERSION`
after a push to `main`. It authenticates its push with a dedicated GitHub App
installation token, not a personal access token. The default `GITHUB_TOKEN`
cannot bypass the required-pull-request rule merely by granting it
`contents: write`.

An organization/repository administrator must complete this setup before merging
changes that enable the App token:

1. Register a dedicated GitHub App under the `openfedem` organization, for example
   `openfedem-version-bump`. Disable webhooks; this App does not need them.
   Grant only the repository permission **Contents: Read and write**.
   **Metadata: Read-only** is included automatically. No organization,
   administration, or workflow-write permissions are needed.
2. Install the App on **only** `openfedem/fedem-foundation`.
3. Generate an App private key. Under repository **Settings > Secrets and
   variables > Actions**, add the App's Client ID as the repository variable
   `VERSION_BUMP_APP_CLIENT_ID`, and the entire private-key PEM as the repository
   secret `VERSION_BUMP_APP_PRIVATE_KEY`. Do not commit the key or share it in
   issues or pull requests.
4. Under **Settings > Branches**, edit the classic protection rule matching
   `main`. Keep **Require a pull request before merging** enabled, enable
   **Allow specified actors to bypass required pull requests**, and add this
   installed App. If **Restrict who can push to matching branches** is enabled,
   also add the App to its allowed push actors.
5. If any branch rulesets also block this push, add the App to the bypass list
   of each relevant ruleset with **Always allow**, not **For pull requests only**.
   A ruleset bypass does not override classic branch protection, or another
   ruleset. Keep the exception as narrow as possible: a classic
   required-pull-request bypass does not bypass other requirements such as
   status checks or signed commits. If such requirements block the version
   commit, an administrator must decide how to satisfy them or configure a
   suitable ruleset exception; do not disable protections for everyone.

The action requests a short-lived, repository-scoped token with Contents write
permission and revokes it at job completion. The built-in `GITHUB_TOKEN` retains
only read permission. The App's bypass permission is not restricted to
`cfg/VERSION`, so protect access to its private key and review changes to
workflows that can use this secret.

Unlike pushes using `GITHUB_TOKEN`, App-authenticated pushes can trigger other
workflows. The versioning job skips a push whose head commit is authored as
`github-actions`, matching the identity it uses for its version commit and
preventing an infinite bump loop. Do not remove that guard or change the commit
author without updating it.

After setup and merge, check that the versioning job successfully pushes one
version commit, that the resulting versioning run skips the bump job, and that
ordinary contributors still cannot push directly to `main`. Re-running the old
failed run uses its old workflow definition; use a new push to `main` containing
the updated workflow instead.

## Issues and Planning

* We use GitHub issues to track bugs and enhancement requests.

* Please provide as much context as possible when you open an issue. The information you provide must be comprehensive enough to reproduce that issue for the assignee.
