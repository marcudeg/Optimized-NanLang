# NanLang Security Policy

## Supported Versions
NanLang is in rapid development. We only provide security patches for the latest stable branch and the upcoming v4.0 pre-release.

| Version | Supported          | Notes                          |
| ------- | ------------------ | ------------------------------ |
| 3.0.2.x   | :white_check_mark: | Current Stable (SIMD/AVX2)     |
| 4.0.x   | :white_check_mark: | In-Development (JIT/Alignment) |
| < 3.0.2   | :x:                | Legacy - Not Supported         |

## Reporting a Vulnerability

**DO NOT open a public issue for security vulnerabilities.**

If you discover a security flaw related to memory sanitization, signal injection, or buffer management:

1. **Direct Contact:** Please report it via GitHub Private Message to **@p1rater** or open a confidential draft PR.
2. **Details Required:** Include a Proof of Concept (PoC) and a hex dump of the affected bytecode if applicable.
3. **Response Time:** We will evaluate the report and respond within **48 hours**.
4. **Disclosure:** Once the fix is merged into the `main` branch, we will publicly credit the researcher in our `CONTRIBUTORS.md`.

## Security Guarantees
* **Pulse Isolation:** Every signal process runs in a dedicated memory-mapped region.
* **Anti-Forensic Wipe:** NanLang automatically clears potential sensitive data from registers after high-priority operations.