#!/usr/bin/env python3
import argparse, json, os, re, sys, time
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
MEM = os.path.join(ROOT, ".ai", "memory", "conversation.jsonl")

def ensure():
    os.makedirs(os.path.dirname(MEM), exist_ok=True)
    if not os.path.exists(MEM):
        open(MEM, "w").close()

def append(role, content):
    ensure()
    entry = {"ts": time.strftime("%Y-%m-%dT%H:%M:%S"), "role": role, "content": content}
    with open(MEM, "a", encoding="utf-8") as f: f.write(json.dumps(entry)+"\n")

def cmd_search(q):
    ensure()
    hits=[]
    with open(MEM, "r", encoding="utf-8") as f:
        for i,line in enumerate(f,1):
            if q.lower() in line.lower():
                hits.append((i,line.strip()))
    for i,l in hits:
        print(f"{i}: {l}")

def export_memory(path):
    ensure()
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(MEM,"r",encoding="utf-8") as f, open(path,"w",encoding="utf-8") as o:
        o.write(f.read())
    print("Exported to", path)

def clear():
    ensure()
    open(MEM,"w").close()
    print("Memory cleared.")

def forget(spec):
    ensure()
    m = re.match(r"(\d+)-(\d+)$", spec)
    if not m:
        print("FORGET expects start-end line range"); return
    s,e = map(int,m.groups())
    lines=open(MEM,"r",encoding="utf-8").read().splitlines()
    keep = [l for idx,l in enumerate(lines,1) if not (s<=idx<=e)]
    open(MEM,"w",encoding="utf-8").write("\n".join(keep)+"\n")
    print(f"Removed lines {s}-{e}")

def redact(pattern):
    ensure()
    import re as _re
    rgx = _re.compile(pattern)
    lines=open(MEM,"r",encoding="utf-8").read().splitlines()
    red = [rgx.sub("[REDACTED]", l) for l in lines]
    open(MEM,"w",encoding="utf-8").write("\n".join(red)+"\n")
    print("Redacted.")

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--search")
    ap.add_argument("--append", nargs=2, metavar=("ROLE","TEXT"))
    ap.add_argument("COMMAND", nargs="?", help="EXPORT_MEMORY|CLEAR_MEMORY|FORGET:<range>|REDACT:<pattern>")
    ap.add_argument("--export-path", default=os.path.join(ROOT, ".ai","memory","export.jsonl"))
    args=ap.parse_args()

    if args.search: cmd_search(args.search); return
    if args.append: append(args.append[0], args.append[1]); return
    if args.COMMAND:
        if args.COMMAND=="EXPORT_MEMORY": export_memory(args.export_path)
        elif args.COMMAND=="CLEAR_MEMORY": clear()
        elif args.COMMAND.startswith("FORGET:"): forget(args.COMMAND.split(":",1)[1])
        elif args.COMMAND.startswith("REDACT:"): redact(args.COMMAND.split(":",1)[1])
        else: print("Unknown command")
        return
    ap.print_help()

if __name__=="__main__":
    main()
