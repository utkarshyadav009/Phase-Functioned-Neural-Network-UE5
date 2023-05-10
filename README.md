Here is a link to the Demo Exe: https://drive.google.com/file/d/1SMRk3Xv9218jNlPghFPIca2_z9Ww2_zi/view?usp=drivesdk

Things that need improvement or fixing:
-The future trajectory doesn't seem to be getting predicted properly, might change it with unreal's motion trajectory plugin. 
-The mesh deforms when applying the joint rotation and location, Even when the position of the joint are locally transformed based on the parent joint location and rotation.
-Controller Suppport 
-Jump animation with height detection and terrain detection. 

# Phase-Functioned-Neural-Network-UE5
Recreating the phase functioned neural network in unreal engine 

Uses Daniel Holden research paper Phase-Functioned Neural Networks for Character Control 
https://theorangeduck.com/media/uploads/other_stuff/phasefunction.pdf


The goal of this project is to recreate this paper in a real-time game engine, the orignal paper uses opengl, with glm and eigen additional libraries 

this repository already has the additional library used in the orignal paper, the libraries are integrated using unreal's uPlugin method. 

