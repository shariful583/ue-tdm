#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TDMCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UInputAction;
struct FInputActionValue;

/**
 * First-person TDM player character.
 * Handles walk/sprint/crouch/jump/ADS movement states and their replication.
 * Input action assets and tuning values are assigned in a Blueprint child (BP_TDMCharacter).
 */
UCLASS()
class TDMSHOOTER_API ATDMCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATDMCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "TDM|Movement")
	bool IsSprinting() const { return bIsSprinting; }

	UFUNCTION(BlueprintPure, Category = "TDM|Movement")
	bool IsAiming() const { return bIsAiming; }

	/** View pitch in [-90, 90]. Valid on all machines (simulated proxies use replicated RemoteViewPitch). */
	UFUNCTION(BlueprintPure, Category = "TDM|Movement")
	float GetAimPitch() const;

	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	// Components

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/** First-person arms (and later weapon) mesh, visible only to the owning player. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> Mesh1P;

	// Input

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	/** True: press crouch to toggle. False: hold to crouch. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bToggleCrouch = true;

	/** True: press ADS to toggle. False: hold to aim. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bToggleAim = false;

	// Movement tuning (cm/s)

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed = 620.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float CrouchSpeed = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float AimWalkSpeed = 240.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float AimCrouchSpeed = 140.f;

	// Camera

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float DefaultFOV = 90.f;

	/** ADS FOV used until weapons (System 2) supply their own. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float AimFOV = 70.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float FOVInterpSpeed = 15.f;

private:
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJumpPressed();
	void OnJumpReleased();
	void OnCrouchPressed();
	void OnCrouchReleased();
	void OnSprintPressed();
	void OnSprintReleased();
	void OnAimPressed();
	void OnAimReleased();

	/** Local entry points: apply immediately (prediction) and forward to server if needed. */
	void SetSprinting(bool bNewSprinting);
	void SetAiming(bool bNewAiming);

	bool CanSprint() const;
	void UpdateMovementSpeed();

	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewSprinting);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bNewAiming);

	UFUNCTION()
	void OnRep_IsSprinting();

	UFUNCTION()
	void OnRep_IsAiming();

	/** Owner predicts locally, so replicated to everyone except the owner. */
	UPROPERTY(ReplicatedUsing = OnRep_IsSprinting)
	bool bIsSprinting = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsAiming)
	bool bIsAiming = false;
};
