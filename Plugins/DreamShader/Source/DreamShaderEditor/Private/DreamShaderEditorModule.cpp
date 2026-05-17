#include "DreamShaderEditorBridge.h"

#include "CoreGlobals.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"

class FDreamShaderEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (IsRunningCommandlet())
		{
			return;
		}

		Bridge = MakeShared<UE::DreamShader::Editor::Private::FDreamShaderEditorBridge, ESPMode::ThreadSafe>();
		Bridge->Startup();
	}

	virtual void ShutdownModule() override
	{
		if (Bridge)
		{
			Bridge->Shutdown();
			Bridge.Reset();
		}
	}

private:
	TSharedPtr<UE::DreamShader::Editor::Private::FDreamShaderEditorBridge, ESPMode::ThreadSafe> Bridge;
};

IMPLEMENT_MODULE(FDreamShaderEditorModule, DreamShaderEditor)
