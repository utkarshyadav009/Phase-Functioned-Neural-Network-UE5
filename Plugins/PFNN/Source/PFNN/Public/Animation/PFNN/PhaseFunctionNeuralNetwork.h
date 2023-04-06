

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DeveloperSettings.h"
#include "ThirdParty/Eigen/Dense"
#include "PhaseFunctionNeuralNetwork.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(PFNN_Logging, Log, All);

UENUM(BlueprintType)
enum class EPFNNMode : uint8 
{
	PM_Constant		UMETA(DisplayName="Constant"),
	PM_Linear		UMETA(DisplayName="Linear"),
	PM_Cubic		UMETA(DisplayName="Cubic")
};

 /*  Network inputs (total 342 elems):
 * 
 *  (12) total 48
 *    0 -  11 = trajectory position X coordinate
 *   12 -  23 = trajectory position Z coordinate
 *   24 -  35 = trajectory direction X coordinate
 *   36 -  47 = trajectory direction Z coordinate
 * 
 *  (12) total 72
 *   48 -  59 = trajectory gait stand
 *   60 -  71 = trajectory gait walk
 *   72 -  83 = trajectory gait jog
 *   84 -  95 = trajectory gait crouch
 *   96 - 107 = trajectory gait jump
 *  108 - 119 = unused, always 0.0. Reason why there isn't 330 elems as in paper.
 *  
 *  (31) total 186
 *  120 - 212 = joint positions (x,y,z). Every axis is on every third place.
 *  213 - 305 = joint velocities (x,y,z). Every axis is on every third place.
 *  
 *  (12) total 36
 *  306 - 317 = trajectory height, right point
 *  318 - 329 = trajectory height, middle point
 *  330 - 341 = trajectory height, left point
 *  
 *  ----------------------------------
 *  Network outputs (total 311 elems):
 *  
 *  0 = ? trajectory position, x axis ? (1950)
 *  1 = ? trajectory position, z axis ? (1950)
 *  2 = ? trajectory direction ?        (1952)
 *  3 = change in phase
 *  4 - 7 = ? something about IK weights ? (1730)
 *  
 *  (6) total 24
 *    8 -  13 = trajectory position, x axis
 *   14 -  19 = trajectory position, z axis
 *   20 -  25 = trajectory direction, x axis
 *   26 -  31 = trajectory direction, z axis
 *  
 *  (31) total 279
 *   32 - 124 = joint positions (x,y,z). Every axis is on every third place.
 *  125 - 217 = joint velocities (x,y,z). Every axis is on every third place.
 *  218 - 310 = joint rotations (x,y,z). Every axis is on every third place.
 */
 
UCLASS(Config=Engine, defaultconfig, meta=(DisplayName="PFNN Settings"))
class PFNN_API UPhaseFunctionNeuralNetwork : public UDeveloperSettings
{
public:
	GENERATED_BODY()
	UPhaseFunctionNeuralNetwork();
	~UPhaseFunctionNeuralNetwork();
	
	/*
	* @Description Load in the Phase Function Neural Network.
	*/
	bool LoadNetworkData(UObject* arg_ContextObject);

	/*
	* @Description Exponential Linear Unit(ELU) activation function
	* @Param[in] arg_X, Base of the Eigen array
	*/
	static void ELU(Eigen::ArrayXf& arg_X);

	/*
	* @Description Linear implementation of the PFNN more computation intensive but requires less memory
	* @Param[out] arg_Out, Calculation output
	* @Param[in] arg_Y0, Output eigen array
	* @Param[in] arg_Y1, Output eigen array
	* @Param[in] arg_MU, Bias
	*/
	static void Linear(Eigen::ArrayXf& arg_Out, const Eigen::ArrayXf& arg_Y0, const Eigen::ArrayXf& arg_Y1, float arg_MU);
	/*
	* @Description Linear implementation of the PFNN more computation intensive but requires less memory
	* @Param[out] arg_Out, Calculation output
	* @Param[in] arg_Y0, Output eigen array
	* @Param[in] arg_Y1, Output eigen array
	* @Param[in] arg_MU, Bias
	*/
	static void Linear(Eigen::ArrayXXf& arg_Out, const Eigen::ArrayXXf& arg_Y0, const Eigen::ArrayXXf& arg_Y1, float arg_MU);

	/*
	* @Description Cubic implementation of the PFNN more memory intensive but faster computation
	* @Param[out] arg_Out, Calculation output
	* @Param[in] arg_Y0, Output eigen array
	* @Param[in] arg_Y1, Output eigen array
	* @Param[in] arg_Y3, Output eigen array
	* @Param[in] arg_MU, Bias
	*/
	static void Cubic(Eigen::ArrayXf& arg_Out, const Eigen::ArrayXf& arg_Y0, const Eigen::ArrayXf& arg_Y1, const Eigen::ArrayXf& arg_Y2, const Eigen::ArrayXf& arg_Y3, float arg_MU);
	/*
	* @Description Linear implementation of the PFNN more computation intensive but requires less memory
	* @Param[out] arg_Out, Calculation output
	* @Param[in] arg_Y0, Output eigen array
	* @Param[in] arg_Y1, Output eigen array
	* @Param[in] arg_Y3, Output eigen array
	* @Param[in] arg_MU, Bias
	*/
	static void Cubic(Eigen::ArrayXXf& arg_Out, const Eigen::ArrayXXf& arg_Y0, const Eigen::ArrayXXf& arg_Y1, const Eigen::ArrayXXf& arg_Y2, const Eigen::ArrayXXf& arg_Y3, float arg_MU);

	/*
	* @Description Makes the neural network predict based on the given phase
	* @Param[in] arg_Phase, Phase to base the prediction upon
	*/
	void Predict(float arg_Phase);

public:

	UPROPERTY(EditAnywhere, Config, Category = "Network", meta = (ConfigRestartRequired = true))
	EPFNNMode Mode;

	//DO NOT CHANGE! These values determine the dimentions of the Neural Network. Changing them without knowing what you are doing will crash the engine.
	enum { XDIM = 342, YDIM = 311, HDIM = 512 };

	// The mean of the input data used for normalization during training of the PFNN.
	Eigen::ArrayXf Xmean;
	// The standard deviation of the input data used for normalization during training of the PFNN.
	Eigen::ArrayXf Xstd;
	// The mean of the output data used for normalization during training of the PFNN.
	Eigen::ArrayXf Ymean;
	// The standard deviation of the output data used for normalization during training of the PFNN.
	Eigen::ArrayXf Ystd;

	/*The weight matrices of the PFNN between the input layerand the first hidden layer,
	between the first hidden layer and the second hidden layer, and 
	between the second hidden layer and the output layer, respectively.*/
	TArray<Eigen::ArrayXXf> W0;
	TArray<Eigen::ArrayXXf> W1;
	TArray<Eigen::ArrayXXf> W2;


	// The bias vectors of the PFNN for the first hidden layer, second hidden layer, and output layer, respectively.
	TArray<Eigen::ArrayXf> b0;
	TArray<Eigen::ArrayXf> b1;
	TArray<Eigen::ArrayXf> b2;


	// The input data for a single frame of the animation.
	Eigen::ArrayXf Xp;

	// The output data for a single frame of the animation.
	Eigen::ArrayXf Yp;

	// The hidden layer activations for the first and second hidden layers, respectively.
	Eigen::ArrayXf H0;
	Eigen::ArrayXf H1;	
	
	/*The weight matrices for the prediction part of the PFNN between the input layerand the first hidden layer,
	between the first hidden layer and the second hidden layer, and between the second hidden layer and the output layer, respectively.*/ 
	Eigen::ArrayXXf W0p;
	Eigen::ArrayXXf W1p;
	Eigen::ArrayXXf W2p;

	// The bias vectors for the prediction part of the PFNN for the first hidden layer, second hidden layer, and output layer, respectively.
	Eigen::ArrayXf b0p;
	Eigen::ArrayXf b1p;
	Eigen::ArrayXf b2p;	
	//Ending of things that you should not change.
};


/*
* the dimensions and initializing the weights and biases of a feedforward neural network 
with three layers: an input layer with 342 nodes, a hidden layer with 512 nodes,
and an output layer with 311 nodes.

The weight matrices W0p, W1p, and W2p define the connections between the nodes in 
each layer. W0p is the weight matrix between the input and hidden layers, 
W1p is the weight matrix between the hidden layer and itself (referred to as recurrent connections), 
and W2p is the weight matrix between the hidden layer and the output layer. 
The biases, b0p, b1p, and b2p, are added to the weighted sum of inputs for each layer.

The dimensions of the weight matrices and biases are determined by the number of nodes in each layer.
The dimensions of W0p are (HDIM, XDIM) because there are HDIM hidden nodes and XDIM input nodes. 
The dimensions of W1p are (HDIM, HDIM) because there are HDIM hidden nodes and the layer is fully connected to itself.
The dimensions of W2p are (YDIM, HDIM) because there are YDIM output nodes and HDIM hidden nodes. 
The dimensions of the bias vectors are HDIM for b0p and b1p, and YDIM for b2p, corresponding to the 
number of nodes in the hidden and output layers, respectively.
*/