#include "LLM/llm.h"
#include <boost/asio/io_context.hpp>
#include <format>
#include <iostream>
#include <llm.h>
#include <boost/asio.hpp>
auto main(void) -> int{
    boost::asio::io_context ioc;
    baiyu::LLM llm;
    llm.test();
    std::cout << std::format("{} {}","World!","Hello") << std::endl;
}