# Injecte la version du firmware (-D MESHCACHING_VERSION) au moment du
# build : celle du tag git sur une release, "v1.2.3-4-gabc123-dirty" sur
# un build intermédiaire, "dev" hors dépôt git.
import subprocess

Import("env")


def git_version():
    try:
        return subprocess.check_output(
            ["git", "describe", "--tags", "--always", "--dirty"],
            text=True,
        ).strip()
    except Exception:
        return "dev"


version = git_version()
print("Version du firmware : %s" % version)
env.Append(CPPDEFINES=[("MESHCACHING_VERSION", env.StringifyMacro(version))])
