Octavio Almanza
oa5967@tamu.edu

The input data was downloaded from facewaretech.com

Incompatible Blendshapes
To illustrate the effects of loading incompatible blendshapes (blendshapes that modify vertices in the same area),
I created an EMOTION line labeled "Incompatible" in the input.txt file. Comment all other EMOTION lines and uncomment the "Incompatible" EMOTION line.
 - This EMOTION is composed of two blendshapes: "Victor_headGEO_OpenSmile.obj" and "Victor_headGEO_Smile.obj"
Both of the blenshapes cause Victor to smile, changing the coordinates of the vertices near the mouth.
When the weights are set to 1, the vertices end up getting translated beyond the limit alloted by the Victor Maya file.


I have completed the FACS stage (I completed the assignment).

I completed the bonus involving rotating the eyes to make Victor look like he is looking left and right.

Please note the following when running the code:
 - The code assumes that there is only ONE EMOTION line in the input.txt file
 - If there are no EMOTION lines in the file, the program will exit
 - Each emotion has to be composed of 2-3 facial action numbers.
 - Although there are several DELTA lines in the input.txt file, the program will
   only load the obj files necessary to represent the EMOTION.