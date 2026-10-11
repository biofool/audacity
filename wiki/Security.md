# Security — audacity

Fork of audacity/audacity. 2026-10 audit: VSTWrapper `strcpy` overflow fixed upstream as https://github.com/audacity/audacity/pull/12453. Open: https://github.com/biofool/audacity/issues/25 (re-scan audacity-tours).

## Standing rules
- Never commit secrets (`.env`, `*.key`, `credentials*.json` are gitignored).
- Never read or log secret values — key names and lengths only.
- Security fixes land reviewed-but-uncommitted first, then roll dev → main.
