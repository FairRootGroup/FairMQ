#include <fairmq/ProgOptions.h>

#include <string>

void configure(fair::mq::ProgOptions& config, fair::mq::ProgOptions* cfg)
{
    // CHECK-MESSAGES: :[[@LINE+1]]:29: warning: 'ProgOptions::GetValue()' is an old accessor; use 'GetProperty()' [fairmq-modernize-progoptions-accessors]
    int const rate = config.GetValue<int>("rate");
    // CHECK-FIXES: int const rate = config.GetProperty<int>("rate", {});
    // CHECK-MESSAGES: :[[@LINE+1]]:35: warning: 'ProgOptions::GetStringValue()' is an old accessor; use 'GetPropertyAsString()' [fairmq-modernize-progoptions-accessors]
    std::string const name = cfg->GetStringValue("name");
    // CHECK-FIXES: std::string const name = cfg->GetPropertyAsString("name", {});
    // CHECK-MESSAGES: :[[@LINE+1]]:12: warning: 'ProgOptions::SetValue()' is an old accessor; use 'SetProperty()' [fairmq-modernize-progoptions-accessors]
    config.SetValue("rate", rate + 1);
    // CHECK-FIXES: config.SetProperty("rate", rate + 1);
    // The result of SetValue() is used: report, but no fix.
    // CHECK-MESSAGES: :[[@LINE+1]]:27: warning: 'ProgOptions::SetValue()' is an old accessor; use 'SetProperty()' [fairmq-modernize-progoptions-accessors]
    int const rc = config.SetValue("name", name);
    // CHECK-FIXES: int const rc = config.SetValue("name", name);
    // The property API is not reported.
    config.SetProperty("rc", rc);
    static_cast<void>(config.GetProperty<int>("rc"));
}
