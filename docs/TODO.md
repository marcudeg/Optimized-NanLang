# NanLang Monorepo Organization TODO
Status: Planned by BLACKBOXAI

1. [x] Create go.mod at root
2. [x] Move npi/cmd/nanlang → cmd/nanlang, extend for unified CLI

3. [x] Move npi/internal/* → internal/
4. [x] Move npi/cmd/npi → cmd/npi (optional)
5. [x] Update src/main.cpp for --manifest JSON/CLI (→ src/nanlang_cpp.cpp)
6. [x] Create unified Makefile
7. [x] Move docs/*.md → docs/
8. [x] Clean artifacts, rm npi/, update paths in confs
9. [ ] Test: make build && ./bin/nanlang --version && ./bin/nanlang build -r req.txt
10. [ ] attempt_completion

Updated on each step.

