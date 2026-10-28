"""检查项目 Markdown 的相对文件链接和源码行号，不依赖第三方包。"""
from pathlib import Path
import re
from urllib.parse import unquote

root = Path(__file__).resolve().parents[1]
documents = [root / "README.md", *sorted((root / "docs").rglob("*.md"))]
checked = 0
errors = []
for document in documents:
    for target in re.findall(r"\[[^\]]+\]\(([^)]+)\)", document.read_text(encoding="utf-8")):
        if "://" in target or target.startswith("#"):
            continue
        relative, _, anchor = unquote(target).partition("#")
        path = (document.parent / relative).resolve()
        if not path.is_file():
            errors.append(f"{document.relative_to(root)}: missing {target}")
            continue
        if not path.is_relative_to(root):
            errors.append(f"{document.relative_to(root)}: outside project {target}")
        if anchor.startswith("L"):
            match = re.fullmatch(r"L(\d+)(?:-L(\d+))?", anchor)
            count = len(path.read_text(encoding="utf-8").splitlines())
            if not match or not 1 <= int(match[1]) <= int(match[2] or match[1]) <= count:
                errors.append(f"{document.relative_to(root)}: invalid lines {target}")
        checked += 1
if errors:
    raise SystemExit("\n".join(errors))
print(f"Checked {checked} local links in {len(documents)} documents")
