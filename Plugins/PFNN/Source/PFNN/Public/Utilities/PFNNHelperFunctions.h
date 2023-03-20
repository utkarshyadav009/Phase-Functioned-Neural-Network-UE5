// Copyright 2018 Sticks & Stones. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include <ThirdParty/glm/glm.hpp>

#include "PFNNHelperFunctions.generated.h"

/**
 * 
 */
UCLASS()
class PFNN_API UPFNNHelperFunctions : public UObject
{
	GENERATED_BODY()
	
public:

	static FVector XYZTranslationToXZY(const FVector& arg_TranslationVector);
	static FVector XYZTranslationToXZY(const glm::vec3& arg_TranslationVector);

	static glm::vec3 XZYTranslationToXYZ(const FVector& arg_TranslationVector);
	static glm::vec3 XZYTranslationToXYZ(const glm::vec3& arg_TranslationVector);

};
