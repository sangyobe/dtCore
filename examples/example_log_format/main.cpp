#include <dtCore/src/dtLog/dtLog.h>
#include <spdlog/fmt/chrono.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ranges.h>
#include <thread>
#include <vector>

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

reference: https://fmt.dev/latest/syntax/
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
    LOG(info).format("{:d} = {:#b} = {:#B} = {:c} = {:#o} = {:#06x} = {:#06X}", 15, 15, 15, 15, 15, 15, 15);

    // ------------------------------------------------------------------------------------
    // arguments by position:
    //
    LOG(info).format("{0}, {1}, {2}", 'a', 'b', 'c');
    // Result: "a, b, c"
    LOG(info).format("{}, {}, {}", 'a', 'b', 'c');
    // Result: "a, b, c"
    LOG(info).format("{2}, {1}, {0}", 'a', 'b', 'c');
    // Result: "c, b, a"
    LOG(info).format("{0}{1}{0}", "abra", "cad"); // arguments' indices can be repeated
    // Result: "abracadabra"

    // ------------------------------------------------------------------------------------
    // Aligning the text and specifying a width:
    //

    LOG(info).format("{:<30}", "left aligned");
    // Result: "left aligned                  "
    LOG(info).format("{:>30}", "right aligned");
    // Result: "                 right aligned"
    LOG(info).format("{:^30}", "centered");
    // Result: "           centered           "
    LOG(info).format("{:*^30}", "centered"); // use '*' as a fill char
    // Result: "***********centered***********"

    // ------------------------------------------------------------------------------------
    // Dynamic width:
    //

    LOG(info).format("{:<{}}", "left aligned", 30);
    // Result: "left aligned                  "

    // ------------------------------------------------------------------------------------
    // Dynamic precision:
    //

    LOG(info).format("{:.{}f}", 3.14, 1);
    // Result: "3.1"

    // ------------------------------------------------------------------------------------
    // Replacing %+f, %-f, and % f and specifying a sign:
    //

    LOG(info).format("{:+f}; {:+f}", 3.14, -3.14); // show it always
    // Result: "+3.140000; -3.140000"
    LOG(info).format("{: f}; {: f}", 3.14, -3.14); // show a space for positive numbers
    // Result: " 3.140000; -3.140000"
    LOG(info).format("{:-f}; {:-f}", 3.14, -3.14); // show only the minus -- same as '{:f}; {:f}'
    // Result: "3.140000; -3.140000"

    // ------------------------------------------------------------------------------------
    // Replacing %x and %o and converting the value to different bases:
    //

    LOG(info).format("int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    // Result: "int: 42;  hex: 2a;  oct: 52; bin: 101010"
    // with 0x or 0 or 0b as prefix:
    LOG(info).format("int: {0:d};  hex: {0:#x};  oct: {0:#o};  bin: {0:#b}", 42);
    // Result: "int: 42;  hex: 0x2a;  oct: 052;  bin: 0b101010"

    // ------------------------------------------------------------------------------------
    // Padded hex byte with prefix and always prints both hex characters:
    //

    LOG(info).format("{:#04x}", 0);
    // Result: "0x00"

    //
    // Box drawing using Unicode fill:
    //

    LOG(info).format(
        "\n"
        "┌{0:─^{2}}┐\n"
        "│{1: ^{2}}│\n"
        "└{0:─^{2}}┘\n",
        "", "Hello, world!", 20);
    // prints:
    // ┌────────────────────┐
    // │   Hello, world!    │
    // └────────────────────┘

    // ------------------------------------------------------------------------------------
    // Using type-specific formatting:
    //
    // #include <fmt/chrono.h>
    //

    auto t = tm();
    t.tm_year = 2010 - 1900;
    t.tm_mon = 7;
    t.tm_mday = 4;
    t.tm_hour = 12;
    t.tm_min = 15;
    t.tm_sec = 58;
    LOG(info).format("{:%Y-%m-%d %H:%M:%S}", t);
    // Prints: 2010-08-04 12:15:58

    // ------------------------------------------------------------------------------------
    // Using the comma as a thousands separator:
    //
    // #include <fmt/format.h>
    //

    // auto s = LOG(info).format(std::locale("en_US.UTF-8"), "{:L}", 1234567890);
    // // s == "1,234,567,890"

    // ------------------------------------------------------------------------------------
    // range format
    //
    // #include <fmt/range.h>
    //

    LOG(info).format("{}", std::vector{10, 20, 30});
    // Output: [10, 20, 30]

    LOG(info).format("{::#x}", std::vector{10, 20, 30});
    // Output: [0xa, 0x14, 0x1e]

    LOG(info).format("{}", std::vector{'h', 'e', 'l', 'l', 'o'});
    // Output: ['h', 'e', 'l', 'l', 'o']

    LOG(info).format("{:n}", std::vector{'h', 'e', 'l', 'l', 'o'});
    // Output: 'h', 'e', 'l', 'l', 'o'

    LOG(info).format("{:s}", std::vector{'h', 'e', 'l', 'l', 'o'});
    // Output: "hello"

    LOG(info).format("{:?s}", std::vector{'h', 'e', 'l', 'l', 'o', '\n'});
    // Output: "hello\n"

    LOG(info).format("{::}", std::vector{'h', 'e', 'l', 'l', 'o'});
    // Output: [h, e, l, l, o]

    LOG(info).format("{::d}", std::vector{'h', 'e', 'l', 'l', 'o'});
    // Output: [104, 101, 108, 108, 111]

    return 0;
}