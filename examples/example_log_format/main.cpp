#include <dtCore/src/dtLog/dtLog.h>
#include <thread>

/*!
 
replacement_field ::= "{" [arg_id] [":" (format_spec | chrono_format_spec)] "}"
arg_id            ::= integer | identifier
integer           ::= digit+
digit             ::= "0"..."9"
identifier        ::= id_start id_continue*
id_start          ::= "a"..."z" | "A"..."Z" | "_"
id_continue       ::= id_start | digit


format_spec ::= [[fill]align][sign]["#"]["0"][width]["." precision]["L"][type]
fill        ::= <a character other than '{' or '}'>
align       ::= "<" | ">" | "^"
sign        ::= "+" | "-" | " "
width       ::= integer | "{" [arg_id] "}"
precision   ::= integer | "{" [arg_id] "}"
type        ::= "a" | "A" | "b" | "B" | "c" | "d" | "e" | "E" | "f" | "F" |
                "g" | "G" | "o" | "p" | "s" | "x" | "X" | "?"

 */
#define _PI_ (3.141592)

int main(int argc, const char **argv)
{
    dt::Log::Initialize(argv[0]);
    dt::Log::SetLogLevel(dt::Log::LogLevel::trace);

    // arg_id (implicite / explicit)
    LOG(info).format("{} {} {} {}", "hello", 1, 2, 3);
    LOG(info).format("{0} {1} {2} {3}", "hello", 1, 2, 3);

    // align
    LOG(info).format("[{:<10}] [{:^10}] [{:>10}]", "hello", "hello", "hello");

    // sign
    LOG(info).format("{:+}/{:+}/{:-}/{:-}/{: }/{: }", 1.5, -1.5, 1.5, -1.5, 1.5, -1.5);

    // precision
    LOG(info).format("{:10.9} {:10.5} {:10.2}", _PI_, _PI_, _PI_);

    // type
    LOG(info).format("{:d} = {:#b} = {:#B} = {:c} = {:o} = {:#x} = {:#X}", 15, 15, 15, 15, 15, 15, 15);

    return 0;
}