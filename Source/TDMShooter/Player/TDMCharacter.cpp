#include "Player/TDMCharacter.h"

#include "TDMShooter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"

ATDMCharacter::ATDMCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 92.f);

	// First-person: body yaw follows controller, camera handles pitch.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(-10.f, 0.f, 64.f));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetFieldOfView(DefaultFOV);

	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh1P"));
	Mesh1P->SetupAttachment(FirstPersonCamera);
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;

	// Full body mesh: seen by other players; owner only sees its shadow.
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->bCastHiddenShadow = true;

	UCharacterMovementComponent* CMC = GetCharacterMovement();
	CMC->GetNavAgentPropertiesRef().bCanCrouch = true;
	CMC->bCanWalkOffLedgesWhenCrouching = true;
	CMC->SetCrouchedHalfHeight(60.f);
	CMC->MaxWalkSpeed = WalkSpeed;
	CMC->MaxWalkSpeedCrouched = CrouchSpeed;
	CMC->JumpZVelocity = 420.f;
	CMC->AirControl = 0.2f;
}

void ATDMCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Pick up values overridden in Blueprint defaults.
	FirstPersonCamera->SetFieldOfView(DefaultFOV);
	UpdateMovementSpeed();
}

void ATDMCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		const float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;
		const float CurrentFOV = FirstPersonCamera->FieldOfView;
		if (!FMath::IsNearlyEqual(CurrentFOV, TargetFOV, 0.01f))
		{
			FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaSeconds, FOVInterpSpeed));
		}
	}
}

void ATDMCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ATDMCharacter, bIsSprinting, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ATDMCharacter, bIsAiming, COND_SkipOwner);
}

float ATDMCharacter::GetAimPitch() const
{
	return FRotator::NormalizeAxis(GetBaseAimRotation().Pitch);
}

void ATDMCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTDM, Error, TEXT("%s requires an EnhancedInputComponent. Check DefaultInput.ini."), *GetName());
		return;
	}

	if (MoveAction)
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::OnMove);
	}
	if (LookAction)
	{
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::OnLook);
	}
	if (JumpAction)
	{
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::OnJumpPressed);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::OnJumpReleased);
	}
	if (CrouchAction)
	{
		EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::OnCrouchPressed);
		EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ThisClass::OnCrouchReleased);
	}
	if (SprintAction)
	{
		EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &ThisClass::OnSprintPressed);
		EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::OnSprintReleased);
	}
	if (AimAction)
	{
		EIC->BindAction(AimAction, ETriggerEvent::Started, this, &ThisClass::OnAimPressed);
		EIC->BindAction(AimAction, ETriggerEvent::Completed, this, &ThisClass::OnAimReleased);
	}
}

void ATDMCharacter::OnMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void ATDMCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void ATDMCharacter::OnJumpPressed()
{
	// Jump from crouch stands up instead (ACharacter can't jump while crouched).
	if (bIsCrouched)
	{
		UnCrouch();
		return;
	}
	Jump();
}

void ATDMCharacter::OnJumpReleased()
{
	StopJumping();
}

void ATDMCharacter::OnCrouchPressed()
{
	if (bToggleCrouch && GetCharacterMovement()->bWantsToCrouch)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ATDMCharacter::OnCrouchReleased()
{
	if (!bToggleCrouch)
	{
		UnCrouch();
	}
}

void ATDMCharacter::OnSprintPressed()
{
	// Sprint cancels crouch and ADS; latest input wins.
	if (GetCharacterMovement()->bWantsToCrouch)
	{
		UnCrouch();
	}
	SetAiming(false);
	SetSprinting(true);
}

void ATDMCharacter::OnSprintReleased()
{
	SetSprinting(false);
}

void ATDMCharacter::OnAimPressed()
{
	SetAiming(bToggleAim ? !bIsAiming : true);
}

void ATDMCharacter::OnAimReleased()
{
	if (!bToggleAim)
	{
		SetAiming(false);
	}
}

void ATDMCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	if (IsLocallyControlled())
	{
		SetSprinting(false);
	}
}

bool ATDMCharacter::CanSprint() const
{
	// Crouch is not checked here: crouched movement uses MaxWalkSpeedCrouched regardless,
	// and checking it on the server would race the uncrouch carried by saved moves.
	return !bIsAiming;
}

void ATDMCharacter::SetSprinting(bool bNewSprinting)
{
	if (bNewSprinting && !CanSprint())
	{
		return;
	}
	if (bIsSprinting == bNewSprinting)
	{
		return;
	}

	bIsSprinting = bNewSprinting;
	UpdateMovementSpeed();

	if (!HasAuthority())
	{
		ServerSetSprinting(bNewSprinting);
	}
}

void ATDMCharacter::SetAiming(bool bNewAiming)
{
	if (bNewAiming)
	{
		SetSprinting(false);
	}
	if (bIsAiming == bNewAiming)
	{
		return;
	}

	bIsAiming = bNewAiming;
	UpdateMovementSpeed();

	if (!HasAuthority())
	{
		ServerSetAiming(bNewAiming);
	}
}

void ATDMCharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
	bIsSprinting = bNewSprinting && CanSprint();
	UpdateMovementSpeed();
}

void ATDMCharacter::ServerSetAiming_Implementation(bool bNewAiming)
{
	bIsAiming = bNewAiming;
	if (bIsAiming)
	{
		bIsSprinting = false;
	}
	UpdateMovementSpeed();
}

void ATDMCharacter::OnRep_IsSprinting()
{
	UpdateMovementSpeed();
}

void ATDMCharacter::OnRep_IsAiming()
{
	UpdateMovementSpeed();
}

void ATDMCharacter::UpdateMovementSpeed()
{
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	CMC->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : (bIsSprinting ? SprintSpeed : WalkSpeed);
	CMC->MaxWalkSpeedCrouched = bIsAiming ? AimCrouchSpeed : CrouchSpeed;
}
