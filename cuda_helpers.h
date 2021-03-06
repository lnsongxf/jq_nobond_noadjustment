#include <cstdio>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>

// This function converts index to subscripts like ind2sub in MATLAB
__host__ __device__
void ind2sub(int length_size, int* siz_vec, int index, int* subs) {
// Purpose:		Converts index to subscripts. i -> [i_1, i_2, ..., i_n]
//
// Input:		length_size = # of coordinates, i.e. how many subscripts you are getting
// 				siz_vec = vector that stores the largest coordinate value for each subscripts. Or the dimensions of matrices
// 				index = the scalar index
//
// Ouput:		subs = the vector stores subscripts
	int done = 0;
	for (int i=length_size-1; i>=0; i--) {
		// Computer the cumulative dimension
		int cumdim = 1;
		for (int j=0; j<=i-1; j++) {
			cumdim *= siz_vec[j];
		};
		int temp_sub = (index - done)/cumdim;
		subs[i] = temp_sub; 
		done += temp_sub*cumdim;
	};
};

// This function fit a valuex x to a grid X of size n.
// The largest value on grid X that is smaller than x is returned ("left grid point" is returned).
__host__ __device__
int fit2grid(const double x, const int n, const double* X) {
	if (x < X[0]) {
		return 0;
	} else if (x > X[n-1]) {
		return n-1;
	} else {
		int left=0; int right=n-1; int mid=(n-1)/2;
		while(right-left>1) {
			mid = (left + right)/2;
			if (X[mid]==x) {
				return mid;
			} else if (X[mid]<x) {
				left = mid;
			} else {
				right = mid;
			};

		};
		return left;
	}
};

// This function fit a valuex x to a "even" grid X of size n. Even means equi-distance among grid points.
// The largest value on grid X that is smaller than x is returned ("left grid point" is returned).
__host__ __device__
int fit2evengrid(const double x, const int n, const double min, const double max) {
	if (x <= min) return 0;
	if (x >= max) return n-1;
	double step = (max-min)/(n-1);
	return floor((x-min)/step);
};
// This function fit a valuex x to a grid X of size n. For std::vector like stuff
// The largest value on grid X that is smaller than x is returned ("left grid point" is returned).
template <class T>
int fit2grid(const double x, const T X) {
	int n = X.size();
	if (x < X[0]) {
		return 0;
	} else if (x > X[n-1]) {
		return n-1;
	} else {
		int left=0; int right=n-1; int mid=(n-1)/2;
		while(right-left>1) {
			mid = (left + right)/2;
			if (X[mid]==x) {
				return mid;
			} else if (X[mid]<x) {
				left = mid;
			} else {
				right = mid;
			};

		};
		return left;
	}
};

// A function template to display vectors, C array style
template <class T>
void display_vec(T vec, int size) {
	for (int i = 0; i < size; i++) {
		std::printf("The %ith element, @[%i] = %f\n", i+1, i, vec[i]);
	};
};

// A function template to display vectors, std::vector style
template <class T>
void display_vec(T vec) {
	int size = vec.size();
	for (int i = 0; i < size; i++) {
		std::printf("The %ith element, @[%i] = %f\n", i+1, i, vec[i]);
	};
};

// A function template to save vectors to file, C array style
template <class T>
void save_vec(T vec, int size, std::string filename ) {
	std::cout << "================================================================================" << std::endl;
	std::cout << "Saving to " << filename << std::endl;
	std::ofstream fileout(filename.c_str(), std::ofstream::trunc);
	for (int i = 0; i < size; i++) {
		fileout << std::setprecision(16) << vec[i] << '\n';
	};
	fileout.close();
	std::cout << "Done!" << std::endl;
	std::cout << "================================================================================" << std::endl;

};

// A function template to save vectors to file, std::vector style
template <class T>
void save_vec(T vec, std::string filename ) {
	std::cout << "================================================================================" << std::endl;
	std::cout << "Saving to " << filename << std::endl;
	int size = vec.size();
	std::ofstream fileout(filename.c_str(), std::ofstream::trunc);
	for (int i = 0; i < size; i++) {
		fileout << std::setprecision(16) << vec[i] << '\n';
	};
	fileout.close();
	std::cout << "Done!" << std::endl;
	std::cout << "================================================================================" << std::endl;
};

// A function template to save vectors to file, C array style
template <class T>
void load_vec(T& vec, int size, std::string filename ) {
	std::cout << "================================================================================" << std::endl;
	std::cout << "Loading from " << filename << std::endl;
	std::ifstream filein(filename.c_str());
	for (int i = 0; i < size; i++) {
		filein >> vec[i]; 
	};
	filein.close();
	std::cout << "Done!" << std::endl;
	std::cout << "================================================================================" << std::endl;
};

// A function template to save vectors to file, C array style
template <class T>
void load_vec(T& vec, std::string filename ) {
	std::cout << "================================================================================" << std::endl;
	std::cout << "Loading from " << filename << std::endl;
	std::ifstream filein(filename.c_str());
	double temp;
	while (filein >> temp) {
		vec.push_back(temp);
	};
	filein.close();
	std::cout << "Done!" << std::endl;
	std::cout << "================================================================================" << std::endl;
};

// Newton's Method with bracketing, i.e. we know on two points the function differs in sign.
// Codes from Numerical Recipes 3rd. Ed.
// BEWARE: The stopping criteria is not right yet.
template <class T>
__host__ __device__
double newton_bracket(T func, const double x1, const double x2, double x0) {
// Purpose: Tries to find a root for function named func. Its first derivative is given by func.prime().
//			It is assumed that func(x1) and func(x2) are different in sign so a root exists within. x0 is the guess.
	const int newton_maxiter = 100;
	const double newton_tol = 1e-3;
	// Checking the bounds: they need to make sense. Or sometimes the bounds are solutions.
	double f1 = func(x1);
	double f2 = func(x2);
	if (f1*f2>0) return -5179394.1; // The different sign assumption violated!
	if (f1 == 0) return x1;
	if (f2 == 0) return x2;

	// Orient the search so that f(xl) < 0
	double xl, xh;
	if (f1 < 0.0) {
		xl = x1;
		xh = x2; 
	} else {
		xh = x1;
		xl = x2;
	};

	// Initialize guess and other things
	double rts = x0;
	double dxold = abs(x2-x1);
	double dx = dxold;
	double f = func(rts);
	double df = func.prime(rts);

	for (int iter = 0; iter < newton_maxiter; iter++) { 
		if ( 
			( ((rts-xh)*df-f)*((rts-xl)*df-f) > 0.0 )   ||	// Bisect if Newton step out of range
			( abs(2.0*f) > abs(dxold*df)  ) // ... or step not decreasing fast enough
		   )
		{
			dxold = dx;
			dx = 0.5*(xh-xl);
			rts += dxold; // undo the newton step
			rts = xl + dx;
			if (xl == rts) return rts;
		} else {
			// If newton step is okay 
			dxold = dx;
			dx = f/df;
			double temp = rts;
			rts -= dx;
			if (temp==rts) return rts;
		};

		// Check for convergence
		if ( abs(dx)/(1+abs(rts+dx)) < newton_tol ) return rts;

		// Compute new f and df for next iteration
		f = func(rts);
		df = func.prime(rts);

		// Maintain the bracket
		if (f < 0.0) {
			xl = rts;
		} else {
			xh = rts;
		};
	};

	return -51709394.2;
};

// "Raw" Newton's Method
// Codes from Numerical Recipes 3rd. Ed.
template <class T>
__host__ __device__
double newton(T func, const double x1, const double x2, double x0) {
// Purpose: Tries to find a root for function named func. Its first derivative is given by func.prime().
//			func is only defined on [x1,x2] We "pull back" when outside. x0 is the guess.
	const int newton_maxiter = 20;
	const double newton_tol = 1e-4;
	// Initialize guess and other things
	double x_old = x0;
	double x = x0;
	double f1 = func(x1);
	double f2 = func(x2);
	if (f1==0) return x1;
	if (f2==0) return x2;
	for (int iter = 0; iter < newton_maxiter; iter++) { 
		x = x_old - func(x)/func.prime(x);

		// Pull back if outside of support
		if (x<=x1) {
			return -51709394.2;
		};
		if (x>=x2) {
			return -51709394.2;
		};

		// Check for convergence
		if ( (abs(x-x_old)/(1+abs(x_old))<newton_tol) && (abs(func(x)) < newton_tol) ) {
		   	return x;
		} else {
			x_old = x;
		};
	};
	return -51709394.2;
};

// Evaluate Chebychev polynomial of any degree
__host__ __device__
double chebypoly(const int p, const double x) {
	switch (p) {
		case 0: // 0-th order Chebyshev Polynomial
			return 1;
		case 1:
			return x;
		case 2:
			return 2*x*x - 1;
		case 3:
			return 4*x*x*x - 3*x;
	}
	
	// When p>=4, apply the recurrence relation
	double lag1 = 4*x*x*x -3*x;
	double lag2 = 2*x*x - 1;
	double lag0;
	int distance = p - 3;
	while (distance >= 1) {
		lag0 = 2*x*lag1 - lag2;
		lag2 = lag1;
		lag1 = lag0;
		distance--;
	};
	return lag0;
};

// Evaluate Chebychev polynomial of any degree
__host__ __device__
int chebyroots(const int p, double* roots) {
	for (int i=0; i<p; i++) {
		double stuff = p - 0.5 - 1*i;
		roots[i] = cos(M_PI*(stuff)/(p));
	};

	// Account for the fact that cos(pi/2) is not exactly zeros
	if (p%2) {
		roots[(p-1)/2] = 0;
	};
	return 0;
};

// Evaluate Chebychev approximation of any degree
__host__ __device__
double chebyeval(int p, double x, double* coeff) {
	// Note that coefficient vector has p+1 values
	double sum = 0;
	for (int i=0; i<=p; i++) {
		sum += coeff[i]*chebypoly(i,x);	
	};
	return sum;
};

// Eval multi-dimensional Chebyshev tensor basis 
// y = sum T_pi(x_i), i = 1,2,...p
__host__ __device__
double chebyeval_multi (const int n_var, double* x, int* size_vec,int* temp_subs, double* coeff) {
	// Note size_vec's elements are p+1 for each var
	int tot_deg = 1;
	for (int i = 0; i < n_var; i++) {
		tot_deg *= (size_vec[i]); // Note there's p+1 coeffs
	};

	double eval = 0;
	for (int index = 0; index < tot_deg; index++) {
		// Perform ind2sub to get current degrees for each var
		ind2sub(n_var, size_vec, index, temp_subs);

		// Find the values at current degrees
		double temp = 1;
		for (int i = 0; i < n_var; i++) {
			// printf("%i th subscript is %i\n",i,temp_subs[i]);
			temp *= chebypoly(temp_subs[i],x[i]);
		};

		// Add to the eval
		eval += (coeff[index]*temp);
	};
	return eval;
};
