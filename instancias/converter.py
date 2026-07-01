import glob

for f in glob.glob("*.stp"):
    lines = [l.split() for l in open(f) if l.strip()]
    p = {s[1]: s[2] for s in lines if s[0] == "TP"}
    open(f[:-4] + ".txt", "w").writelines(
        f"{s[1]}:{p.get(s[1],0)} {s[2]}:{p.get(s[2],0)} {s[3]}\n"
        for s in lines
        if s[0] == "E"
    )

print(
    "std::vector<std::string> listaGrafos = {\n"
    + ",\n".join(f'    "{f[:-4]}.txt"' for f in glob.glob("*.stp"))
    + "\n};"
)