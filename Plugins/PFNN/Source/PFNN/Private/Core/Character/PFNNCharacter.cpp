

#include "Core/Character/PFNNCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Animation/AnimComponents/TrajectoryComponent.h"
#include "Animation/AnimInstances/PFNNAnimInstance.h"
#define print(msg, ...) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT(msg), __VA_ARGS__))

// Sets default values
APFNNCharacter::APFNNCharacter() : PFNNAnimInstance(nullptr)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	//GetCharacterMovement()->RotationRate = FRotator(0.0f, 300.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 200.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	//PFNN Animationn system Tracjectory class initialisation 
	TrajectoryComponent = CreateDefaultSubobject<UTrajectoryComponent>(TEXT("TrajectoryComponent"));

	bIsSkeletonDebuggingEnabled = false;

    PFNNJointTransformControlRig.SetNum(18);
    JointNameByIndex.SetNum(18);

    bIsPFNNLoaded = false;

}


//////////////////////////////////////////////////////////////////////////
// Input
void APFNNCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

    PlayerInputComponent->BindAction("Jog", IE_Pressed, this, &APFNNCharacter::OnJogPressed);
    PlayerInputComponent->BindAction("Jog", IE_Released, this, &APFNNCharacter::OnJogReleased);

    PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &APFNNCharacter::OnCrouchPressed);
    PlayerInputComponent->BindAction("Crouch", IE_Released, this, &APFNNCharacter::OnCrouchReleased);


	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &APFNNCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &APFNNCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &APFNNCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &APFNNCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &APFNNCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &APFNNCharacter::TouchStopped);
}

void APFNNCharacter::OnJogPressed()
{
    TrajectoryComponent->JogActivated = 1.0f;
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed *= 2.0f; // Increase the movement speed by a factor, for example, 2
    }
}

void APFNNCharacter::OnJogReleased()
{
    TrajectoryComponent->JogActivated = 0.0f;
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed /= 2.0f; // Reset the movement speed back to normal
    }
}

void APFNNCharacter::OnCrouchPressed()
{
    TrajectoryComponent->CrouchActivated = 1.0f;
}

void APFNNCharacter::OnCrouchReleased()
{
    TrajectoryComponent->CrouchActivated = 0.0f;
}




void APFNNCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void APFNNCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void APFNNCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void APFNNCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void APFNNCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void APFNNCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

UTrajectoryComponent* APFNNCharacter::GetTrajectoryComponent()
{
	return TrajectoryComponent;
}

bool APFNNCharacter::HasDebuggingEnabled()
{
	return bIsSkeletonDebuggingEnabled;
}

void APFNNCharacter::SetJointTransformForControlRig(FTransform JointTransform, int index, FName JointName)
{
    switch (index)
    {
    case 3:
        PFNNJointTransformControlRig[0] = JointTransform;
        JointNameByIndex[0] = JointName;
        break;
    case 4:
        PFNNJointTransformControlRig[1] = JointTransform;
        JointNameByIndex[1] = JointName;
        break;
    case 5:
        PFNNJointTransformControlRig[2] = JointTransform;
        JointNameByIndex[2] = JointName;
        break;
    case 6:
        PFNNJointTransformControlRig[3] = JointTransform;
        JointNameByIndex[3] = JointName;
        break;
    case 8:
        PFNNJointTransformControlRig[4] = JointTransform;
        JointNameByIndex[4] = JointName;
        break;
    case 9:
        PFNNJointTransformControlRig[5] = JointTransform;
        JointNameByIndex[5] = JointName;
        break;
    case 10:
        PFNNJointTransformControlRig[6] = JointTransform;
        JointNameByIndex[6] = JointName;
        break;
    case 11:
        PFNNJointTransformControlRig[7] = JointTransform;
        JointNameByIndex[7] = JointName;
        break;
    case 13:
        PFNNJointTransformControlRig[8] = JointTransform;
        JointNameByIndex[8] = JointName;
        break;
    case 14:
        PFNNJointTransformControlRig[9] = JointTransform;
        JointNameByIndex[9] = JointName;
        break;
    case 15:
        PFNNJointTransformControlRig[10] = JointTransform;
        JointNameByIndex[10] = JointName;
        break;
    case 17:
        PFNNJointTransformControlRig[11] = JointTransform;
        JointNameByIndex[11] = JointName;
        break;
    case 19:
        PFNNJointTransformControlRig[12] = JointTransform;
        JointNameByIndex[12] = JointName;
        break;
    case 20:
        PFNNJointTransformControlRig[13] = JointTransform;
        JointNameByIndex[13] = JointName;
        break;
    case 21:
        PFNNJointTransformControlRig[14] = JointTransform;
        JointNameByIndex[14] = JointName;
        break;
    case 26:
        PFNNJointTransformControlRig[15] = JointTransform;
        JointNameByIndex[15] = JointName;
        break;
    case 27:
        PFNNJointTransformControlRig[16] = JointTransform;
        JointNameByIndex[16] = JointName;
        break;
    case 28:
        PFNNJointTransformControlRig[17] = JointTransform;
        JointNameByIndex[17] = JointName;
        break;
    default:
        // Do nothing for any other index value
        break;
    }
}

void APFNNCharacter::SetPFNNLoaded(bool bPFNNLoaded)
{
    bIsPFNNLoaded = bPFNNLoaded;
}


void APFNNCharacter::GetPFNNLoaded(bool& bPFNNLoaded)
{
    bPFNNLoaded = bIsPFNNLoaded;
}

void APFNNCharacter::GetLeftLegJointTransform(FTransform& LeftUpLeg, FTransform& LeftLeg, FTransform& LeftFoot, FTransform& LeftToeBase)
{
    LeftUpLeg = PFNNJointTransformControlRig[0];
    LeftLeg = PFNNJointTransformControlRig[1];
    LeftFoot = PFNNJointTransformControlRig[2];
    LeftToeBase = PFNNJointTransformControlRig[3];
}

void APFNNCharacter::GetRightLegJointTransform(FTransform& RightUpLeg, FTransform& RightLeg, FTransform& RightFoot, FTransform& RightToeBase)
{
    RightUpLeg = PFNNJointTransformControlRig[4];
    RightLeg = PFNNJointTransformControlRig[5];
    RightFoot = PFNNJointTransformControlRig[6];
    RightToeBase = PFNNJointTransformControlRig[7];
}

void APFNNCharacter::GetLeftArmJointTransform(FTransform& LeftArm, FTransform& LeftForeArm, FTransform& LeftHand)
{
    LeftArm = PFNNJointTransformControlRig[15];
    LeftForeArm = PFNNJointTransformControlRig[16];
    LeftHand = PFNNJointTransformControlRig[17];
}

void APFNNCharacter::GetRightArmJointTransform(FTransform& RightArm, FTransform& RightForeArm, FTransform& RightHand)
{
    RightArm = PFNNJointTransformControlRig[12];
    RightForeArm = PFNNJointTransformControlRig[13];
    RightHand = PFNNJointTransformControlRig[14];
}

void APFNNCharacter::GetSpineJointTransform(FTransform& Spine, FTransform& Spine1, FTransform& Neck, FTransform& Head)
{
    Spine = PFNNJointTransformControlRig[8];
    Spine1 = PFNNJointTransformControlRig[9];
    Neck = PFNNJointTransformControlRig[10];
    Head = PFNNJointTransformControlRig[11];
}