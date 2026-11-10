#!/usr/bin/env python3
################################################################################
#    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################
"""Runs fairmq-clangd as a language server on one test input.

The test passes if clangd publishes a diagnostic of the given check and offers
a quick fix for it, i.e. the fairmq-* checks work in an editor.

Usage: ClangdCheck.py <fairmq-clangd> <check> <input> <flags file> <workdir>
"""

import json
import pathlib
import shutil
import subprocess
import sys


def main():
    clangd, check, source, flags, workdir = sys.argv[1:6]
    work = pathlib.Path(workdir)
    shutil.rmtree(work, ignore_errors=True)
    work.mkdir(parents=True)
    src = work / pathlib.Path(source).name
    shutil.copy(source, src)
    args = [line for line in pathlib.Path(flags).read_text().splitlines() if line]
    (work / "compile_commands.json").write_text(json.dumps(
        [{"directory": str(work), "file": str(src), "arguments": ["clang++", *args, "-c", str(src)]}]))
    (work / ".clangd").write_text(
        "Diagnostics:\n  ClangTidy:\n    Add: [%s]\n    FastCheckFilter: Loose\n" % check)

    proc = subprocess.Popen([clangd, "--log=error", f"--compile-commands-dir={work}"],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    next_id = 0

    def send(method, params, notification=False):
        nonlocal next_id
        msg = {"jsonrpc": "2.0", "method": method, "params": params}
        if not notification:
            next_id += 1
            msg["id"] = next_id
        body = json.dumps(msg).encode()
        proc.stdin.write(b"Content-Length: %d\r\n\r\n" % len(body) + body)
        proc.stdin.flush()
        return next_id

    def receive(predicate):
        while True:
            length = None
            while True:
                line = proc.stdout.readline()
                if not line:
                    raise RuntimeError("clangd exited")
                if line in (b"\r\n", b"\n"):
                    break
                if line.lower().startswith(b"content-length:"):
                    length = int(line.split(b":")[1])
            msg = json.loads(proc.stdout.read(length))
            if predicate(msg):
                return msg

    uri = src.as_uri()
    try:
        init = send("initialize", {
            "processId": None,
            "rootUri": work.as_uri(),
            "capabilities": {"textDocument": {"codeAction": {"codeActionLiteralSupport": {
                "codeActionKind": {"valueSet": ["quickfix"]}}}}},
        })
        receive(lambda m: m.get("id") == init)
        send("initialized", {}, notification=True)
        send("textDocument/didOpen", {"textDocument": {
            "uri": uri, "languageId": "cpp", "version": 1, "text": src.read_text()}},
            notification=True)
        diags = receive(lambda m: m.get("method") == "textDocument/publishDiagnostics"
                        and m["params"]["uri"] == uri)["params"]["diagnostics"]
        found = [d for d in diags if d.get("code") == check]
        if not found:
            print(f"no {check} diagnostic, got:", json.dumps(diags, indent=1))
            return 1
        req = send("textDocument/codeAction", {
            "textDocument": {"uri": uri}, "range": found[0]["range"],
            "context": {"diagnostics": [found[0]]}})
        actions = receive(lambda m: m.get("id") == req)["result"]
        if not any(a.get("kind") == "quickfix" and "edit" in a for a in actions):
            print("no quick fix, got:", json.dumps(actions, indent=1))
            return 1
        print(f"{len(found)} {check} diagnostics, quick fix: {actions[0]['title']}")
        send("shutdown", None)
        return 0
    finally:
        proc.kill()


if __name__ == "__main__":
    sys.exit(main())
