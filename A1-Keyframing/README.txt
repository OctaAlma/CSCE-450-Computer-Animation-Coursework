Octavio Almanza
628005492
oa5967@tamu.edu

My tmax value from task 3 is 5.0.
The tmax declaration is done at line 61.

For my custom function, I made it so the helicopter would go backwards through the spline.
To create this function, I solved the following system of linear equations using normalized t and s values:

0a + 0b + 0c + d = 0.0
(0.3^3)a + (0.3^2)b + (0.3)c + 1d = 0.6
(0.4^3)a + (0.4^2)b + (0.4)c + 1d = 0.5
a + b + c + d = 1.0

When solved, I got the following function:

s(t) = 10.119*t^3 + -14.5833*t^2 + 5.46429*t + 0

I did not do the bonus.

Note: My code has a bug where the interpolated helicopter will not
follow the spline unless the first control point is at (0,0,0).
In other words, In order for the code to behave as expected, the first
control point must be at (0,0,0).

Note: To modify the control points, go to the initControlPoints() function
in line 63