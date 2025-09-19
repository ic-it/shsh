# ShSh - Shitty Shell

A shitty command-line shell written in C with remote command execution support.

## Features

- Interactive mode (REPL) and script execution
- Command pipes
- Background process execution
- Input/output redirection
- TCP server for remote command execution
- Built-in commands: `cd`, `exit`, `jobs`

## Syntax

```bash
# Comments
;           # Command separator
|           # Pipe
&           # Background execution
>           # Redirect stdout to file
<           # Redirect stdin from file
'...'       # Escape special characters
>@ host:port # TCP output redirection (not implemented)
<@ host:port # TCP input redirection (not implemented)
```

## Grammar

```ebnf
<program> ::= <command> ( ";" )? | <command> ";" <program>

<command> ::= <bg_command> | <piped_command> | <command_body>

<piped_command> ::= <command_body> "|" <command>
<bg_command> ::= <command_body> "&"

<command_body> ::= <command_word> <arguments> <io_redirection>?

<arguments> ::= ( " " <argument> )*
<argument> ::= <unescaped_word> | <escaped_word>

<io_redirection> ::= <file_io> | <tcp_io>
<file_io> ::= ">" <filename> | "<" <filename>
<tcp_io> ::= ">@" <ip>:<port> | "<@" <ip>:<port>

<escaped_word> ::= "'" <any_chars> "'"
<unescaped_word> ::= <non_special_chars>+
```

## Usage

### Local mode
```bash
# Interactive mode
./shsh

# Execute script
./shsh script.sh
```

### Server mode
```bash
# Start server
./shsh -s -p 8080

# Connect client
nc 127.0.0.1 8080
```

### Command examples
```bash
# Simple commands
echo "Hello World"
ls -la

# Pipes
ls -la | grep txt

# Background execution
sleep 10 &

# Redirection
echo "test" > output.txt
cat < input.txt

# Command sequences
cd /tmp; pwd; ls
```

## Build

```bash
make            # Debug build
make RELEASE=1  # Release build
make clean      # Clean
```

## Limitations

- TCP redirection (`>@`, `<@`) is declared in grammar but not implemented
- Command substitution (\`\`) is not supported
- Wildcard expansion (`*`) doesn't work
- Escape characters (`\`) work partially

## Author

Illia Chaban <ic-it@mail.com>