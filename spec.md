# ShSh 
> C written shell

## Syntax
# - Comment  
;  - Command separator  
\>  - Redirect stdout  
<  - Redirect stdin  
|  - Pipe  
\  - Escape character  
&  - Run in background
`` - Command substitution  
\>&`<n>` - Redirect to the file descriptor `<n>`  ???   
'' - escape all special characters  
\>@ - Redirect to tcp socket
<@  - Redirect from tcp socket
\*  - match any filename part


## Grammar
```ebnf
/* ls -l|grep '*.txt'>output.txt&;echo hello>@192.168.1.10:8080 */

/* Program */
<program> ::= 
    <command> ( ";" )?
    | <command> ";" <program>
    | <command> "\n" <program>

/* Command -- Command and arguments (optional) */
<command> ::=
    <bg_command>
    | <piped_command>
    | <command_body>

/* Piped command */
<piped_command> ::= <command_body> "|" <command>

/* Background command */
<bg_command> ::= <command_body> "&"

/* Command body */
<command_body> ::= 
    <command_word> <arguments>
    | <command_word> <arguments> ">" <filename>
    | <command_word> <arguments> "<" <filename>
    | <command_word> <arguments> ">@" <dstaddr>
    | <command_word> <arguments> "<@" <dstaddr>


/* Arguments -- optional */
<arguments> ::=
    E
    | " " <argument> <arguments>

/* Argument */
<argument> ::=
    <unescaped_word>
    | <escaped_word>

/* Filename - It can be unescaped or escaped */
<filename> ::= 
    <unescaped_word>
    | <escaped_word>

/* Unescaped filename is a string of any non-special characters and '*' */
<unescaped_word> ::=
    <non_special_chars> ( <unescaped_word> )?
    | "*" ( <unescaped_word> )?

/* Escaped filename is a string of any characters enclosed in single quotes */
<escaped_word> ::= "'" <all_chars> "'"

/* Commandword - A string of any characters non-special characters */
<command_word> ::= <non_special_chars>

/* Destination address */
<dstaddr> ::= <ip> ":" <port>
<ip> ::= [0-9]+ "." [0-9]+ "." [0-9]+ "." [0-9]+
<port> ::= [0-9]+

/* All Non-special characters (TMP)*/
<non_special_chars> ::= ( [a-z] | [A-Z] | [0-9] | "." | "-" )+

/* All characters (TMP)*/
<all_chars> ::= ( [a-z] | [A-Z] | [0-9] | "." | "-" | ";" | ">" | "<" | "|" | "&" | "*" )+

```
