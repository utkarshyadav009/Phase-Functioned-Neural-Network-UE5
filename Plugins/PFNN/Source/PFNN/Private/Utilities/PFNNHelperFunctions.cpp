


#include "Utilities/PFNNHelperFunctions.h"

FVector UPFNNHelperFunctions::XYZTranslationToXZY(const FVector& arg_TranslationVector)
{
	return FVector(arg_TranslationVector.X, arg_TranslationVector.Z, arg_TranslationVector.Y);
}

FVector UPFNNHelperFunctions::XYZTranslationToXZY(const glm::vec3& arg_TranslationVector)
{
	return FVector(arg_TranslationVector.x, arg_TranslationVector.z, arg_TranslationVector.y);
}

glm::vec3 UPFNNHelperFunctions::XZYTranslationToXYZ(const FVector& arg_TranslationVector)
{
	return glm::vec3(arg_TranslationVector.X, arg_TranslationVector.Z, arg_TranslationVector.Y);
}

glm::vec3 UPFNNHelperFunctions::XZYTranslationToXYZ(const glm::vec3& arg_TranslationVector)
{
	return glm::vec3(arg_TranslationVector.x, arg_TranslationVector.z, arg_TranslationVector.y);
}

