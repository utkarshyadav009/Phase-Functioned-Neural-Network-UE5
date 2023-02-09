// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <ThirdParty/glm/glm.hpp>

#include <ThirdParty/glm/glm.hpp>
#include <ThirdParty/glm/gtx/transform.hpp>
#include <ThirdParty/glm/gtx/euler_angles.hpp>

#include "PFNNHelperFunctions.generated.h"

/**
 * 
 */
UCLASS()
class PFNN_ANIMATIONS_API UPFNNHelperFunctions : public UObject
{
	GENERATED_BODY()

public:

	static FVector XYZTranslationToXZY(const FVector& arg_TranslationVector);
	static FVector XYZTranslationToXZY(const glm::vec3& arg_TranslationVector);

	static glm::vec3 XZYTranslationToXYZ(const FVector& arg_TranslationVector);
	static glm::vec3 XZYTranslationToXYZ(const glm::vec3& arg_TranslationVector);
};

 // Lazy destroy file handle
class PFNN_ANIMATIONS_API UPFNNHelperFileReader
{
	UPFNNHelperFileReader() = delete;

public:
	UPFNNHelperFileReader(const FString arg_FileName, ...);
	~UPFNNHelperFileReader();
	
	FFloat32 readItem();
	bool isOpen();

private:
	IFileHandle* m_FileHandle;
};
