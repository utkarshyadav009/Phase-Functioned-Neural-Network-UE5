// Fill out your copyright notice in the Description page of Project Settings.
#include "PFNNHelperFunctions.h"
#include "PhaseFunctionNeuralNetwork.h"

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

FFloat32 UPFNNHelperFileReader::readItem()
{
	FFloat32 item;
	uint8* ByteBuffer = reinterpret_cast<uint8*>(&item);

	m_FileHandle->Read(ByteBuffer, sizeof(FFloat32));
	return item;
}

bool UPFNNHelperFileReader::isOpen()
{
	return nullptr != m_FileHandle;
}

UPFNNHelperFileReader::UPFNNHelperFileReader(const FString arg_FileName, ...)
{
	FString RelativePath = FPaths::ProjectDir();
	const FString FullPath = RelativePath += arg_FileName;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	m_FileHandle = PlatformFile.OpenRead(*FullPath);
}

UPFNNHelperFileReader::~UPFNNHelperFileReader()
{
	delete m_FileHandle;
}
