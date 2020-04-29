// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionContext.h"


namespace AppInstaller::CLI::Execution
{
    namespace
    {
        // The context that will receive CTRL signals
        Context* s_contextForCtrlHandler = nullptr;

        BOOL WINAPI CtrlHandlerForContext(DWORD ctrlType)
        {
            AICLI_LOG(CLI, Info, << "Got CTRL type: " << ctrlType);

            // Won't save us from every crash, but a few more than direct access.
            Context* context = s_contextForCtrlHandler;
            if (!context)
            {
                return FALSE;
            }

            switch (ctrlType)
            {
            case CTRL_C_EVENT:
            case CTRL_BREAK_EVENT:
                context->Terminate(E_ABORT);
                context->Reporter.CancelInProgressTask(false);
                return TRUE;
                // According to MSDN, we should never receive these due to having gdi32/user32 loaded in our process.
                // But handle them as a force terminate anyway.
            case CTRL_CLOSE_EVENT:
            case CTRL_LOGOFF_EVENT:
            case CTRL_SHUTDOWN_EVENT:
                context->Terminate(E_ABORT);
                context->Reporter.CancelInProgressTask(true);
                return TRUE;
            default:
                return FALSE;
            }
        }

        void SetCtrlHandlerContext(Context* context)
        {
            // Only one is allowed right now.
            THROW_HR_IF(E_UNEXPECTED, s_contextForCtrlHandler != nullptr && context != nullptr);

            if (context == nullptr)
            {
                LOG_IF_WIN32_BOOL_FALSE(SetConsoleCtrlHandler(CtrlHandlerForContext, FALSE));
                s_contextForCtrlHandler = nullptr;
            }
            else
            {
                s_contextForCtrlHandler = context;
                LOG_IF_WIN32_BOOL_FALSE(SetConsoleCtrlHandler(CtrlHandlerForContext, TRUE));
            }
        }
    }

    Context::~Context()
    {
        if (m_disableCtrlHandlerOnExit)
        {
            EnableCtrlHandler(false);
        }
    }

    void Context::EnableCtrlHandler(bool enabled)
    {
        SetCtrlHandlerContext(enabled ? this : nullptr);
        m_disableCtrlHandlerOnExit = enabled;
    }

    void Context::UpdateForArgs()
    {
        if (Args.Contains(Args::Type::NoVT))
        {
            Reporter.SetStyle(VisualStyle::NoVT);
        }
        else if (Args.Contains(Args::Type::PlainStyle))
        {
            Reporter.SetStyle(VisualStyle::Plain);
        }
        else if (Args.Contains(Args::Type::RainbowStyle))
        {
            Reporter.SetStyle(VisualStyle::Rainbow);
        }
    }
}