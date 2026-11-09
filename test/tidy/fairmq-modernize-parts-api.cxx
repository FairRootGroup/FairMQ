#include <fairmq/Message.h>
#include <fairmq/Parts.h>

#include <cstddef>

std::size_t sizes(fair::mq::Parts& parts, fair::mq::Message* raw)
{
    // CHECK-MESSAGES: :[[@LINE+1]]:11: warning: 'Parts::AddPart(Message*)' is deprecated; pass a 'fair::mq::MessagePtr' [fairmq-modernize-parts-api]
    parts.AddPart(raw);
    // CHECK-FIXES: parts.AddPart(fair::mq::MessagePtr(raw));
    // CHECK-MESSAGES: :[[@LINE+1]]:33: warning: 'Parts::AtRef()' is deprecated; use 'Parts::At()' [fairmq-modernize-parts-api]
    std::size_t const a = parts.AtRef(0).GetSize();
    // CHECK-FIXES: std::size_t const a = parts.At(0)->GetSize();
    // CHECK-MESSAGES: :[[@LINE+1]]:38: warning: 'Parts::AtRef()' is deprecated; use 'Parts::At()' [fairmq-modernize-parts-api]
    fair::mq::Message& first = parts.AtRef(0);
    // CHECK-FIXES: fair::mq::Message& first = *parts.At(0);
    // The current API is not reported.
    parts.AddPart(fair::mq::MessagePtr(nullptr));
    std::size_t const b = parts.At(0)->GetSize() + first.GetSize();
    return a + b;
}
