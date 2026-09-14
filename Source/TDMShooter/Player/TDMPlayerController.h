#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TDMPlayerController.generated.h"

class UInputMappingContext;

/**
 * TDM player controller. Registers the Enhanced Input mapping context for local players.
 * Mapping context asset is assigned in a Blueprint child (BP_TDMPlayerController).
 */
UCLASS()
class TDMSHOOTER_API ATDMPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 DefaultMappingPriority = 0;
};
