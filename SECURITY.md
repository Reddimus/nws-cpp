# Security Policy

`nws-cpp` is a third-party C++ client for the US National Weather
Service public API. The API itself is unauthenticated, but consumers
of this library may run it inside services that handle credentials or
sensitive operator data. This file is the canonical contact path for
reporting a vulnerability in the client.

## Supported Versions

Security fixes are made on the latest published `vX.Y.Z` tag. Older
tags are not back-patched — bump your `FetchContent_Declare(... GIT_TAG ...)`
pin or your `find_package(nws-cpp X.Y.Z REQUIRED)` constraint to the
latest minor on the same major as part of the upgrade.

| Version    | Supported          |
| ---------- | ------------------ |
| latest tag | :white_check_mark: |
| older      | :x:                |

## Reporting a Vulnerability

**Do not open a public issue.** Use GitHub's [private vulnerability
reporting](https://github.com/Reddimus/nws-cpp/security/advisories/new)
flow, which delivers the report to the maintainer privately and tracks
coordinated disclosure.

When reporting, please include:

- Affected version (tag or commit SHA)
- A reproduction — minimal code or test case
- Impact (memory corruption / DoS / something else)
- Whether you've notified anyone else

You can expect:

- Acknowledgement within **3 business days**
- An initial assessment + severity rating within **7 business days**
- A fix on a new `vX.Y.Z+1` tag, or a clear timeline if the fix is
  larger

## Out of Scope

- Bugs against `api.weather.gov` itself — those go to NOAA's own
  vulnerability program, not this client library.
- Operational issues (rate-limit handling, network blips) — file a
  regular issue.
- Theoretical issues against dependencies — report them upstream
  (`openssl`, `libcurl`, `nlohmann/json`, `googletest`). We pin via
  FetchContent and bump on credible advisories.
