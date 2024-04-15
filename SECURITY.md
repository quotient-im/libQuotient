# Security Policy

## Supported Versions

| Version | Supported                     |
| ------- | ----------------------------- |
| dev     | :white_check_mark: (unstable) |
| 0.8.x   | :white_check_mark:            |
| 0.7.x   | :white_check_mark:            |
| older   | :x:                           |

## Reporting a Vulnerability

If you find a vulnerability, or evidence of one, use either of the following contacts:
- via email: [Kitsune Ral](mailto:Kitsune-Ral@users.sf.net); or
- via Matrix: [direct chat with @kitsune:matrix.org](https://matrix.to/#/@kitsune:matrix.org?action=chat).

In any of these two options, indicate that you have such information (do not share it yet), and we'll tell you the next steps.

By default, we will give credit to anyone who reports a vulnerability in a responsible way so that we can fix it before public disclosure.
If you want to remain anonymous or pseudonymous instead, please let us know; we will gladly respect your wishes.
NEVER provide a fix as a PR upfront and be very cautious about pushing fixes
to public repos (e.g. at non-private GitHub), as all of this can reveal
the vulnerability itself.

## Timeline and commitments

Initial reaction to the message about a vulnerability (see above) will be
no more than 5 days. From the moment of the private report or public disclosure
(if it hasn't been reported earlier in private) of each vulnerability, we take
effort to fix it on priority before any other issues. In case of vulnerabilities
with [CVSS v2](https://nvd.nist.gov/cvss.cfm) score of 4.0 and higher
the commitment is to provide a workaround within 30 days and a full fix
within 60 days after the project has been made aware about the vulnerability
(in private or in public). For vulnerabilities with lower score there is
no commitment on the timeline, only prioritisation. The full fix doesn't imply
that all software functionality remains accessible (in the worst case
the vulnerable functionality may be disabled or removed to prevent the attack).
