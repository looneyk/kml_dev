# A PostToolUse hook for Edit, Write and MultiEdit. Checks the comments, string literals and document text that an edit
# adds for the entries in watch_words.txt, and reports each match to Claude through stderr with exit code 2. Only files
# tracked by git are checked, and files that Write creates and git does not ignore. Any failure of the check itself exits
# 0, so it never blocks an edit.

import json
import os
import re
import subprocess
import sys

HOOK_DIRECTORY = os.path.dirname(os.path.abspath(__file__))

C_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl", ".ipp", ".cs", ".js", ".ts", ".glsl",
                ".vert", ".frag", ".comp", ".hlsl"}
HASH_EXTENSIONS = {".py", ".sh", ".ps1", ".cmake", ".gd", ".yml", ".yaml", ".toml"}
TEXT_EXTENSIONS = {".md", ".txt"}

STRING_PATTERN = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
HASH_STRING_PATTERN = re.compile(r'"((?:[^"\\\n]|\\.)*)"|\'((?:[^\'\\\n]|\\.)*)\'')
INLINE_CODE_PATTERN = re.compile(r"`[^`]*`")


def read_list(filename):
    entries = []
    with open(os.path.join(HOOK_DIRECTORY, filename), "r", encoding="utf-8") as stream:
        for line in stream:
            line = line.strip()
            if line and not line.startswith("#"):
                entries.append(line)
    return entries


def compile_watch_list():
    prose_patterns = []
    comment_patterns = []
    for entry in read_list("watch_words.txt"):
        if entry.startswith("comment-re:"):
            comment_patterns.append((entry[len("comment-re:"):], re.compile(entry[len("comment-re:"):], re.IGNORECASE)))
        elif entry.startswith("re:"):
            prose_patterns.append((entry[len("re:"):], re.compile(entry[len("re:"):], re.IGNORECASE)))
        else:
            words = r"\s+".join(re.escape(word) for word in entry.split())
            prose_patterns.append((entry, re.compile(r"(?<![\w-])" + words + r"(?![\w-])", re.IGNORECASE)))
    return prose_patterns, comment_patterns


def compile_allowed_phrases():
    return [re.compile(r"(?<![\w-])" + r"\s+".join(re.escape(word) for word in phrase.split()) + r"(?![\w-])",
                       re.IGNORECASE)
            for phrase in read_list("allowed_phrases.txt")]


def run_git(directory, *arguments):
    return subprocess.run(["git", *arguments], cwd=directory, capture_output=True, text=True).returncode


def in_scope(filepath, tool_name):
    directory = os.path.dirname(filepath)
    if not os.path.isdir(directory):
        return False
    if run_git(directory, "ls-files", "--error-unmatch", "--", filepath) == 0:
        return True
    # An untracked file is checked only when Write has just created it and git does not ignore it
    return tool_name == "Write" and run_git(directory, "check-ignore", "-q", "--", filepath) == 1


def added_lines(tool_name, tool_input):
    if tool_name == "Write":
        return tool_input.get("content", "").splitlines()
    edits = tool_input.get("edits") if tool_name == "MultiEdit" else [tool_input]
    lines = []
    for edit in edits or []:
        old_lines = set(edit.get("old_string", "").splitlines())
        lines.extend(line for line in edit.get("new_string", "").splitlines() if line not in old_lines)
    return lines


def file_syntax(filepath):
    name = os.path.basename(filepath)
    extension = os.path.splitext(name)[1].lower()
    if name == "CMakeLists.txt" or extension in HASH_EXTENSIONS:
        return "hash"
    if extension in C_EXTENSIONS:
        return "c"
    if extension in TEXT_EXTENSIONS:
        return "text"
    return None


def prose_parts(line, syntax):
    """Returns (prose, comment) for one line: the text to check with every pattern, and the comment text alone."""
    if syntax == "text":
        return INLINE_CODE_PATTERN.sub("", line), ""
    if syntax == "c":
        strings = [match.group(1) for match in STRING_PATTERN.finditer(line)]
        code = STRING_PATTERN.sub('""', line)
        comment = ""
        if "//" in code:
            comment = code[code.index("//") + 2:]
        elif code.lstrip().startswith(("/*", "*")):
            comment = code.lstrip().lstrip("/*").rstrip("*/")
    else:
        strings = [match.group(1) or match.group(2) or "" for match in HASH_STRING_PATTERN.finditer(line)]
        code = HASH_STRING_PATTERN.sub('""', line)
        comment = code[code.index("#") + 1:] if "#" in code else ""
    return " ".join([comment] + strings), comment


def line_number(file_lines, line):
    for index, file_line in enumerate(file_lines):
        if file_line.rstrip() == line.rstrip():
            return index + 1
    return None


def main():
    payload = json.loads(sys.stdin.buffer.read().decode("utf-8"))
    tool_name = payload.get("tool_name", "")
    tool_input = payload.get("tool_input", {})
    filepath = tool_input.get("file_path", "")
    if not filepath or tool_name not in ("Edit", "Write", "MultiEdit"):
        return 0
    filepath = os.path.abspath(filepath)
    normalized = filepath.replace("\\", "/")
    if "/.claude/hooks/" in normalized or os.path.basename(filepath) == "CLAUDE.md":
        return 0
    syntax = file_syntax(filepath)
    if syntax is None or not in_scope(filepath, tool_name):
        return 0

    prose_patterns, comment_patterns = compile_watch_list()
    allowed_phrases = compile_allowed_phrases()
    try:
        with open(filepath, "r", encoding="utf-8", errors="replace") as stream:
            file_lines = stream.read().splitlines()
    except OSError:
        file_lines = []

    findings = []
    for line in added_lines(tool_name, tool_input):
        prose, comment = prose_parts(line, syntax)
        for phrase in allowed_phrases:
            prose = phrase.sub(" ", prose)
        patterns = [(pattern, prose) for _, pattern in prose_patterns]
        if syntax != "text" and comment:
            patterns += [(pattern, comment) for _, pattern in comment_patterns]
        matched = []
        for pattern, text in patterns:
            match = pattern.search(text)
            if match:
                matched.append(f"'{match.group(0).strip()}'")
        if matched:
            number = line_number(file_lines, line)
            location = f"{normalized}:{number}" if number else normalized
            findings.append(f"  {location}: {', '.join(matched)}\n    {line.strip()}")

    if not findings:
        return 0
    message = ("Wording check: the text just added has watch-list words. For each, keep it only if the word is used in its "
               "ordinary sense; otherwise reword it to say literally what the code does.\n" + "\n".join(findings) + "\n")
    sys.stderr.buffer.write(message.encode("utf-8", errors="replace"))
    return 2


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception:
        sys.exit(0)
