"""A tiny declarative command layer over :mod:`argparse`.

This is the Typer replacement. It exists so ``cli.py`` can stay a flat list of
declarations — command name, options, handler — instead of a hand-rolled
subparser tree with a dispatch ``if`` chain, while the only import is stdlib.

What it provides beyond bare argparse:
  * ``@app.command`` / ``@group.command`` registration with the handler attached,
    so there is no separate dispatch table to keep in sync;
  * a themed top-level help that lists commands with one-line summaries, grouped
    the way the CLI is meant to be read rather than alphabetically;
  * ``--`` passthrough: everything after a bare ``--`` reaches the handler as
    ``args.forwarded`` instead of being parsed;
  * exit codes as return values — a handler returns an int, never calls sys.exit.
"""

import argparse
import sys

from . import console


class Opt:
    """One argparse argument, declared next to the command that uses it."""

    def __init__(self, *names, **kwargs):
        self.names = names
        self.kwargs = kwargs

    def add_to(self, parser):
        parser.add_argument(*self.names, **self.kwargs)


class _Help(argparse.RawDescriptionHelpFormatter):
    """argparse's layout with colored section headings and usage prefix."""

    def start_section(self, heading):
        super().start_section(console.paint(console.theme.header, heading)
                              if heading else heading)

    def add_usage(self, usage, actions, groups, prefix=None):
        if prefix is None:
            prefix = console.paint(console.theme.header, "usage:") + " "
        super().add_usage(usage, actions, groups, prefix)


class _Command:
    def __init__(self, name, handler, help_text, options, aliases, forwards):
        self.name = name
        self.handler = handler
        self.help = help_text
        self.options = options
        self.aliases = aliases
        self.forwards = forwards


class _Registry:
    """Shared registration behaviour for the app and its command groups."""

    def __init__(self):
        self.commands = []

    def command(self, name, help=None, options=(), aliases=(), forwards=False):
        """Register a handler as a command.

        *forwards* documents that the command accepts trailing ``-- args``; the
        split itself happens in :meth:`App.run` regardless, so a handler can
        always read ``args.forwarded``.
        """
        def decorator(handler):
            self.commands.append(_Command(
                name, handler, help or (handler.__doc__ or "").strip(),
                list(options), tuple(aliases), forwards))
            return handler
        return decorator


class Group(_Registry):
    """A namespace of subcommands, e.g. ``ds docker build``."""

    def __init__(self, name, help_text):
        super().__init__()
        self.name = name
        self.help = help_text


class App(_Registry):
    def __init__(self, prog, description, version=None, epilog=None):
        super().__init__()
        self.prog = prog
        self.description = description
        self.version = version
        self.epilog = epilog
        self.groups = []
        self.sections = []  # (title, [command names]) for the grouped help

    def group(self, name, help=None):
        grp = Group(name, help or "")
        self.groups.append(grp)
        return grp

    def section(self, title, *names):
        """Declare how commands are grouped in the top-level help."""
        self.sections.append((title, list(names)))

    def _build_parser(self):
        parser = argparse.ArgumentParser(
            prog=self.prog, description=self.description,
            epilog=self.epilog, formatter_class=_Help, add_help=True)
        if self.version:
            parser.add_argument("--version", action="version",
                                version=f"{self.prog} {self.version}")
        sub = parser.add_subparsers(dest="_command", metavar="<command>")
        for cmd in self.commands:
            self._add_command(sub, cmd)
        for grp in self.groups:
            grp_parser = sub.add_parser(grp.name, help=grp.help,
                                        description=grp.help,
                                        formatter_class=_Help)
            grp_sub = grp_parser.add_subparsers(dest="_action",
                                                metavar="<action>")
            for cmd in grp.commands:
                self._add_command(grp_sub, cmd, group=grp.name)
            grp_parser.set_defaults(_group_parser=grp_parser)
        return parser

    def _add_command(self, sub, cmd, group=None):
        summary = cmd.help.splitlines()[0] if cmd.help else ""
        parser = sub.add_parser(
            cmd.name, help=summary, description=cmd.help,
            aliases=list(cmd.aliases), formatter_class=_Help,
            epilog=("Arguments after `--` are forwarded to the target."
                    if cmd.forwards else None))
        for opt in cmd.options:
            opt.add_to(parser)
        parser.set_defaults(_handler=cmd.handler,
                            _command=group or cmd.name,
                            _action=cmd.name if group else None)

    def print_help(self):
        def accent(text):
            return console.paint(console.theme.header, text)

        print(f"{accent(self.description)}\n")
        print(f"{accent('usage:')} {self.prog} <command> [options]\n")

        listed = {name for _, names in self.sections for name in names}
        by_name = {c.name: c for c in self.commands}
        sections = list(self.sections)
        leftover = [c.name for c in self.commands if c.name not in listed]
        if leftover:
            sections.append(("Other", leftover))

        width = max([len(n) for n in by_name] + [len(g.name) for g in self.groups]
                    + [8])
        for title, names in sections:
            print(accent(title))
            for name in names:
                cmd = by_name.get(name)
                if cmd is None:
                    continue
                summary = cmd.help.splitlines()[0] if cmd.help else ""
                print(f"  {name.ljust(width)}  {summary}")
            print()
        if self.groups:
            print(accent("Command groups"))
            for grp in self.groups:
                actions = ", ".join(c.name for c in grp.commands)
                print(f"  {grp.name.ljust(width)}  {grp.help} [{actions}]")
            print()
        print(console.paint(console.theme.muted,
                            f"Run `{self.prog} <command> --help` for a "
                            f"command's options."))

    def run(self, argv=None):
        argv = list(sys.argv[1:] if argv is None else argv)
        argv, forwarded = _split_forwarded(argv)

        if not argv or argv[0] in ("help", "-h", "--help"):
            if len(argv) > 1:
                return self._build_parser().parse_args([argv[1], "--help"])
            self.print_help()
            return 0

        parser = self._build_parser()
        args = parser.parse_args(argv)
        args.forwarded = forwarded

        handler = getattr(args, "_handler", None)
        if handler is None:
            # A group invoked with no action: show that group's own help rather
            # than the top-level one, which would not list its actions.
            group_parser = getattr(args, "_group_parser", None)
            (group_parser or parser).print_help()
            return 0
        return handler(args) or 0


def _split_forwarded(argv):
    """Split *argv* at the first bare ``--``; the tail is passed through."""
    if "--" in argv:
        index = argv.index("--")
        return argv[:index], argv[index + 1:]
    return argv, []


def main(app, argv=None):
    """Entry point wrapper: turns the usual failure modes into exit codes."""
    try:
        return app.run(argv)
    except KeyboardInterrupt:
        console.warn("Interrupted.")
        return 130
    except SystemExit as exc:  # find_project_root and argparse both raise this
        code = exc.code
        if isinstance(code, str):
            console.error(code)
            return 1
        return code or 0
