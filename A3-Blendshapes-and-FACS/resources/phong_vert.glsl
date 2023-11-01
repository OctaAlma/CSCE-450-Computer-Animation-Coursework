#version 120

attribute vec4 aPos;
attribute vec3 aNor;
attribute vec2 aTex;

// Blendshape position and normal vectors:
attribute vec3 blendshape1Pos;
attribute vec3 blendshape1Nor;

attribute vec3 blendshape2Pos;
attribute vec3 blendshape2Nor;

attribute vec3 blendshape3Pos;
attribute vec3 blendshape3Nor;

uniform mat4 P;
uniform mat4 MV;

// Weights for each of the aforementioned blendshapes
uniform float wA;
uniform float wB;
uniform float wC;

varying vec3 vPos;
varying vec3 vNor;
varying vec2 vTex;

void main()
{	
	vec4 posCam = MV * vec4(aPos.xyz + blendshape1Pos * wA + blendshape2Pos * wB + blendshape3Pos * wC, 1.0);
	vec3 norCam = (MV * vec4(normalize(aNor + blendshape1Nor * wA + blendshape2Nor * wB + blendshape3Nor * wC), 0.0)).xyz;
	gl_Position = P * posCam;
	
	vPos = posCam.xyz; 
	vNor = norCam;
	vTex = aTex;
}
