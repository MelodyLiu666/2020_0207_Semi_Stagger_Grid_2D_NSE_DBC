// ConsoleApplication1.cpp: 定义控制台应用程序的入口点。
//

// 2020_0205_Semi_Stagger_Grid_2D_NSE_DBC_P_Part.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <time.h>
#include "string.h"
#include <iostream>
#include <math.h>
#include <fstream>

using namespace std;

#define dt 0.0125
#define N_t 1
#define PI 3.1415926536

#define DA   2.0
#define DALPHA PI
#define DBETA  2.0

#define rho     1.0
#define KINVIS  1.0 //kinetic viscosity 
#define Re      1.0 / KINVIS 

#define gamma_0 1.0

void Set_Grid_And_Constant_Equ_Params(
	double *aw, double *ae, double *an, double *as, double *ap,
	double *igrid_x, double *igrid_y, int GridX, int GridY,
	double dx_h, double dy_h) {

	for (int iy = 1; iy < (GridY - 1); iy++)
	{
		for (int ix = 0; ix < GridX; ix++)
		{
			int i = ix + iy * GridX;

			if ((ix == 0) || (ix == (GridX - 1))) igrid_x[i] = 0.5;
			else igrid_x[i] = 1.0;

			if ((iy == 1) || (iy == (GridY - 2))) igrid_y[i] = 0.5;
			else igrid_y[i] = 1.0;

			double dx = igrid_x[i] * dx_h, dy = igrid_y[i] * dy_h, dV = dx * dy;

			aw[i] = igrid_y[i], ae[i] = aw[i], an[i] = igrid_x[i], as[i] = an[i];

			if (ix == 0) aw[i] = 0;
			if (ix == (GridX - 1)) ae[i] = 0;
			if (iy == 1) an[i] = 0;
			if (iy == (GridY - 2)) as[i] = 0;

			ap[i] = aw[i] + ae[i] + an[i] + as[i];
		}
	}
}


void Real_Load(
	double *u_real, double *v_real, double *p_real,
	int GridX_Central, int GridY_Central,
	double dx_h, double dy_h, int iter_start_step) {

	int N = GridX_Central - 1;

	for (int iy = 1; iy < (GridY_Central - 1); iy++)
	{
		for (int ix = 0; ix < GridX_Central; ix++)
		{
			int i = ix + iy * GridX_Central;

			double y = (iy - (N / 2) - 1) * dy_h, x = ix * dx_h;

			double t = iter_start_step * dt;

			u_real[i] = DA * cos(PI*y)*sin(DALPHA*x)*sin(DBETA*t);
			v_real[i] = -(DA*DALPHA / PI)*sin(PI*y)*cos(DALPHA*x)*sin(DBETA*t);
			p_real[i] = DA * sin(PI*y)*sin(DALPHA*x)*cos(DBETA*t);
		}
	}

}


void Boundary_Load_on_Central(
	double *u_c, double *v_c, double *u_real,
	double *v_real, int GridX_Central, int GridY_Central) {

	for (int iy = 1; iy < (GridY_Central - 1); iy++)
	{
		int i = iy * GridX_Central;
		int i_2 = iy * GridX_Central + (GridX_Central - 1);

		u_c[i] = u_real[i];
		v_c[i] = v_real[i];

		u_c[i_2] = u_real[i_2];
		v_c[i_2] = v_real[i_2];

	}

	for (int ix = 0; ix < GridX_Central; ix++)
	{
		int i = ix + GridX_Central;
		int i_2 = ix + GridX_Central * (GridY_Central - 2);

		u_c[i] = u_real[i];
		v_c[i] = v_real[i];

		u_c[i_2] = u_real[i_2];
		v_c[i_2] = v_real[i_2];
	}
}


void Init_U(
	double *u_n, double *u_c, int GridX_U, int GridY_U) {

	for (int ix = 0; ix < GridX_U; ix++)
	{
		for (int iy = 2; iy < (GridY_U - 2); iy++)
		{
			int i = ix + iy * GridX_U;

			u_n[i] = 0.5 * (u_c[i] + u_c[i - GridX_U]);
		}

		int i_0 = ix + GridX_U, i_1 = ix + (GridY_U - 2)* GridX_U;

		u_n[i_0] = 2 * u_c[i_0] - u_n[i_0 + GridX_U];
		u_n[i_1] = 2 * u_c[i_1] - u_n[i_1 - GridX_U];

	}
}


void Init_V(
	double *v_n, double *v_c, int GridX_V, int GridY_V) {

	for (int iy = 1; iy < (GridY_V - 1); iy++)
	{
		for (int ix = 1; ix < (GridX_V - 1); ix++)
		{
			int i = ix + iy * GridX_V;

			v_n[i] = 0.5 * (v_c[i] + v_c[i - 1]);
		}

		int i_0 = iy * GridX_V, i_1 = (GridX_V - 1) + iy * GridX_V;

		v_n[i_0] = 2 * v_c[i_0] - v_n[i_0 + 1];
		v_n[i_1] = 2 * v_c[i_1] - v_n[i_1 - 1];
	}
}

void Get_Source_U(
	double *F_u, int GridX_U, int GridY_U, double dx_h, double dy_h, int i_t) {

	int N = GridX_U - 1;

	for (int iy = 2; iy < (GridY_U - 2); iy++)
	{
		for (int ix = 0; ix < GridX_U; ix++)
		{
			int i = ix + iy * GridX_U;

			double y = (iy - (N / 2) - 1 - 1 + 0.5) * dy_h, x = ix * dx_h;

			double t = i_t * dt;

			F_u[i] = (DA*DBETA*cos(PI*y)*sin(DALPHA*x)*cos(DBETA*t)
				+ DA * DA*DALPHA*sin(DALPHA*x)*cos(DALPHA*x)*sin(DBETA*t)*sin(DBETA*t))
				+ DA * DALPHA*sin(PI*y)*cos(DALPHA*x)*cos(DBETA*t)
				+ KINVIS * (DA*(PI*PI + DALPHA * DALPHA)*cos(PI*y)*sin(DALPHA*x)*sin(DBETA*t));
		}
	}

}

void Get_Source_V(
	double *F_v, int GridX_V, int GridY_V, double dx_h, double dy_h, int i_t) {

	int N = GridX_V - 1 - 1;

	for (int iy = 1; iy < (GridY_V - 1); iy++)
	{
		for (int ix = 1; ix < (GridX_V - 1); ix++)
		{
			int i = ix + iy * GridX_V;

			double y = (iy - (N / 2) - 1) * dy_h, x = (ix - 1 + 0.5) * dx_h;

			double t = i_t * dt;

			F_v[i] = ((-DA * DALPHA*DBETA / PI)*sin(PI*y)*cos(DALPHA*x)*cos(DBETA*t)
				+ (DA*DA*DALPHA*DALPHA / PI)*cos(PI*y)*sin(PI*y)*sin(DBETA*t)*sin(DBETA*t))
				+ (PI*DA)*cos(PI*y)*sin(DALPHA*x)*cos(DBETA*t)
				- KINVIS * ((DA*DALPHA*(DALPHA*DALPHA + PI * PI) / PI)*sin(PI*y)*cos(DALPHA*x)*sin(DBETA*t));
		}
	}

}


void Gu_Update(double *G_u, double *F_u, double *u_n, double *v_n, int GridX_U, int GridY_U, int GridX_V, int GridY_V, double dx_h, double dy_h) {

	double Cu = 0, S = 0;
	double Fw = 0, Fe = 0, Fn = 0, Fs = 0;
	double aw = 0, ae = 0, an = 0, as = 0, ap = 0;

	double inv_dV = 1.0 / (dx_h * dy_h);
	bool alpha_w = 0, alpha_e = 0, alpha_n = 0, alpha_s = 0;

	for (int iy = 2; iy < (GridY_U - 2); iy++)
	{
		for (int ix = 0; ix < GridX_U; ix++)
		{
			int i = ix + iy * GridX_U;

			Fw = dy_h * (u_n[i] + u_n[i - 1]) * 0.5;
			Fe = dy_h * (u_n[i] + u_n[i + 1]) * 0.5;
			Fn = dx_h * (v_n[i - GridX_V] + v_n[i + 1 - GridX_V]) / 2;
			Fs = dx_h * (v_n[i] + v_n[i + 1]) / 2;

			alpha_w = 0, alpha_e = 0, alpha_n = 0, alpha_s = 0;
			if (Fw > 0) alpha_w = 1;
			if (Fe > 0) alpha_e = 1;
			if (Fn > 0) alpha_n = 1;
			if (Fs > 0) alpha_s = 1;

			aw = alpha_w * Fw, ae = -(1 - alpha_e) * Fe;
			an = alpha_n * Fn, as = -(1 - alpha_s) * Fs;

			//add the Fi limits
			if (ix == 0) aw = 0, Fw = 0;
			if (ix == (GridX_U - 1)) ae = 0, Fe = 0;

			ap = aw + ae + an + as + (Fe - Fw) + (Fs - Fn);

			S = 0;
			if ((ix > 1) && (ix < (GridX_U - 2)))
				S = alpha_w * Fw * (3 * u_n[i] - 2 * u_n[i - 1] - u_n[i - 2])
				+ alpha_e * Fe * (u_n[i - 1] + 2 * u_n[i] - 3 * u_n[i + 1])
				+ (1 - alpha_w) * Fw * (3 * u_n[i - 1] - 2 * u_n[i] - u_n[i + 1])
				+ (1 - alpha_e) * Fe * (2 * u_n[i + 1] + u_n[i + 2] - 3 * u_n[i]);

			if ((iy > 2) && (iy < (GridY_U - 3)))
				S += alpha_n * Fn * (3 * u_n[i] - 2 * u_n[i - GridX_U] - u_n[i - 2 * GridX_U])
				+ alpha_s * Fs * (u_n[i - GridX_U] + 2 * u_n[i] - 3 * u_n[i + GridX_U])
				+ (1 - alpha_n) * Fn * (3 * u_n[i - GridX_U] - 2 * u_n[i] - u_n[i + GridX_U])
				+ (1 - alpha_s) * Fs * (2 * u_n[i + GridX_U] + u_n[i + 2 * GridX_U] - 3 * u_n[i]);

			Cu = ap * u_n[i] - aw * u_n[i - 1] - ae * u_n[i + 1] - an * u_n[i - GridX_U] - as * u_n[i + GridX_U] - S * 0.125;

			G_u[i] = F_u[i] + u_n[i] / dt - Cu * inv_dV;

			//G_u_real[i] = F_u[i] + u_hat[i] / dt - l_u_C;
			//G_v_real[i] = F_v[i] + v_hat[i] / dt - l_v_C;
			//G_u_real[i] = F_u[i] + u_real / dt - l_u_C;
			//G_v_real[i] = F_v[i] + v_real / dt - l_v_C;
			//G_u[i] = G_u_real[i];
			//G_v[i] = G_v_real[i];

			//Dufile << G_u[i] << "  ";
			//luDfile << G_u_real[i] << "  ";
			//Dufile << (G_u[i] - G_u_real[i]) << "  ";
		}
		//Dufile << endl;
		//luDfile << endl;
	}

}

void Gv_Update(double *G_v, double *F_v, double *u_n, double *v_n, int GridX_V, int GridY_V, double dx_h, double dy_h) 
{

	double Cu = 0, S = 0;
	double Fw = 0, Fe = 0, Fn = 0, Fs = 0;
	double aw = 0, ae = 0, an = 0, as = 0, ap = 0;

	double inv_dV = 1.0 / (dx_h * dy_h);
	bool alpha_w = 0, alpha_e = 0, alpha_n = 0, alpha_s = 0;

	for (int iy = 2; iy < (GridY_U - 2); iy++)
	{
		for (int ix = 0; ix < GridX_U; ix++)
		{
			int i = ix + iy * GridX_U;

			Fw = dy_h * (u_n[i] + u_n[i - 1]) * 0.5;
			Fe = dy_h * (u_n[i] + u_n[i + 1]) * 0.5;
			Fn = dx_h * (v_n[i - GridX_V] + v_n[i + 1 - GridX_V]) / 2;
			Fs = dx_h * (v_n[i] + v_n[i + 1]) / 2;

			alpha_w = 0, alpha_e = 0, alpha_n = 0, alpha_s = 0;
			if (Fw > 0) alpha_w = 1;
			if (Fe > 0) alpha_e = 1;
			if (Fn > 0) alpha_n = 1;
			if (Fs > 0) alpha_s = 1;

			aw = alpha_w * Fw, ae = -(1 - alpha_e) * Fe;
			an = alpha_n * Fn, as = -(1 - alpha_s) * Fs;

			//add the Fi limits
			if (ix == 0) aw = 0, Fw = 0;
			if (ix == (GridX_U - 1)) ae = 0, Fe = 0;

			ap = aw + ae + an + as + (Fe - Fw) + (Fs - Fn);

			S = 0;
			if ((ix > 1) && (ix < (GridX_U - 2)))
				S = alpha_w * Fw * (3 * u_n[i] - 2 * u_n[i - 1] - u_n[i - 2])
				+ alpha_e * Fe * (u_n[i - 1] + 2 * u_n[i] - 3 * u_n[i + 1])
				+ (1 - alpha_w) * Fw * (3 * u_n[i - 1] - 2 * u_n[i] - u_n[i + 1])
				+ (1 - alpha_e) * Fe * (2 * u_n[i + 1] + u_n[i + 2] - 3 * u_n[i]);

			if ((iy > 2) && (iy < (GridY_U - 3)))
				S += alpha_n * Fn * (3 * u_n[i] - 2 * u_n[i - GridX_U] - u_n[i - 2 * GridX_U])
				+ alpha_s * Fs * (u_n[i - GridX_U] + 2 * u_n[i] - 3 * u_n[i + GridX_U])
				+ (1 - alpha_n) * Fn * (3 * u_n[i - GridX_U] - 2 * u_n[i] - u_n[i + GridX_U])
				+ (1 - alpha_s) * Fs * (2 * u_n[i + GridX_U] + u_n[i + 2 * GridX_U] - 3 * u_n[i]);

			Cu = ap * u_n[i] - aw * u_n[i - 1] - ae * u_n[i + 1] - an * u_n[i - GridX_U] - as * u_n[i + GridX_U] - S * 0.125;

			G_u[i] = F_u[i] + u_n[i] / dt - Cu * inv_dV;

			//G_u_real[i] = F_u[i] + u_hat[i] / dt - l_u_C;
			//G_v_real[i] = F_v[i] + v_hat[i] / dt - l_v_C;
			//G_u_real[i] = F_u[i] + u_real / dt - l_u_C;
			//G_v_real[i] = F_v[i] + v_real / dt - l_v_C;
			//G_u[i] = G_u_real[i];
			//G_v[i] = G_v_real[i];

			//Dufile << G_u[i] << "  ";
			//luDfile << G_u_real[i] << "  ";
			//Dufile << (G_u[i] - G_u_real[i]) << "  ";
		}
		//Dufile << endl;
		//luDfile << endl;
	}
}

int main()
{
	int N = 4, N_halo = 2;

	double dx_h = 2.0 / N, dy_h = dx_h;

	int GridX_Central = N + 1, GridY_Central = N + N_halo + 1;

	int N_elem_Central = GridX_Central * GridY_Central;

	int N_Mem_tot_Central = ((N_elem_Central - 1) / 32 + 1) * 32;

	double *u_c = new double[N_Mem_tot_Central]; //store the result on Domain Grid
	double *v_c = new double[N_Mem_tot_Central];
	double *p_c = new double[N_Mem_tot_Central];

	memset(u_c, 0, N_Mem_tot_Central * sizeof(double));
	memset(v_c, 0, N_Mem_tot_Central * sizeof(double));
	memset(p_c, 0, N_Mem_tot_Central * sizeof(double));

	int GridX_U = GridX_Central, GridY_U = GridY_Central + 1;
	int GridX_V = GridX_Central + 1, GridY_V = GridY_Central;
	int GridX_P = GridX_Central + 1, GridY_P = GridY_Central + 1;

	int N_elem_U = GridX_U * GridY_U;
	int N_elem_V = GridX_V * GridY_V;
	int N_elem_P = GridX_P * GridY_P;

	int N_Mem_tot_U = ((N_elem_U - 1) / 32 + 1) * 32;
	int N_Mem_tot_V = ((N_elem_V - 1) / 32 + 1) * 32;
	int N_Mem_tot_P = ((N_elem_P - 1) / 32 + 1) * 32;

	double *u = new double[N_Mem_tot_U]; //store the result on t_n+1
	double *u_n = new double[N_Mem_tot_U]; //store the u* on t-n
	double *u_s = new double[N_Mem_tot_U];
	double *u_0 = new double[N_Mem_tot_U];
	double *u_b = new double[N_Mem_tot_U];
	double *u_hat = new double[N_Mem_tot_U];
	double *u_star = new double[N_Mem_tot_U];

	memset(u, 0, N_Mem_tot_U * sizeof(double));
	memset(u_n, 0, N_Mem_tot_U * sizeof(double));
	memset(u_s, 0, N_Mem_tot_U * sizeof(double));
	memset(u_0, 0, N_Mem_tot_U * sizeof(double));
	memset(u_b, 0, N_Mem_tot_U * sizeof(double));
	memset(u_hat, 0, N_Mem_tot_U * sizeof(double));
	memset(u_star, 0, N_Mem_tot_U * sizeof(double));

	double *v = new double[N_Mem_tot_V]; //store the result on t_n+1
	double *v_n = new double[N_Mem_tot_V]; //store the u on t-n
	double *v_s = new double[N_Mem_tot_V];
	double *v_0 = new double[N_Mem_tot_V];
	double *v_b = new double[N_Mem_tot_V];
	double *v_hat = new double[N_Mem_tot_V];
	double *v_star = new double[N_Mem_tot_V];

	memset(v, 0, N_Mem_tot_V * sizeof(double));
	memset(v_n, 0, N_Mem_tot_V * sizeof(double));
	memset(v_s, 0, N_Mem_tot_V * sizeof(double));
	memset(v_0, 0, N_Mem_tot_V * sizeof(double));
	memset(v_b, 0, N_Mem_tot_V * sizeof(double));
	memset(v_hat, 0, N_Mem_tot_V * sizeof(double));
	memset(v_star, 0, N_Mem_tot_V * sizeof(double));

	double *p = new double[N_Mem_tot_P];
	double *p_0 = new double[N_Mem_tot_P];
	double *p_s = new double[N_Mem_tot_P];
	double *p_b = new double[N_Mem_tot_P];

	memset(p, 0, N_Mem_tot_P * sizeof(double));
	memset(p_0, 0, N_Mem_tot_P * sizeof(double));
	memset(p_s, 0, N_Mem_tot_P * sizeof(double));
	memset(p_b, 0, N_Mem_tot_P * sizeof(double));

	double *igrid_x_U = new double[N_Mem_tot_U]; //store the grid-setting
	double *igrid_y_U = new double[N_Mem_tot_U];

	double *igrid_x_V = new double[N_Mem_tot_V]; //store the grid-setting
	double *igrid_y_V = new double[N_Mem_tot_V];

	double *igrid_x_P = new double[N_Mem_tot_P]; //store the grid-setting for scaler vector
	double *igrid_y_P = new double[N_Mem_tot_P];

	memset(igrid_x_U, 0, N_Mem_tot_U * sizeof(double));
	memset(igrid_y_U, 0, N_Mem_tot_U * sizeof(double));
	memset(igrid_x_V, 0, N_Mem_tot_V * sizeof(double));
	memset(igrid_y_V, 0, N_Mem_tot_V * sizeof(double));
	memset(igrid_x_P, 0, N_Mem_tot_P * sizeof(double));
	memset(igrid_y_P, 0, N_Mem_tot_P * sizeof(double));

	double *aw_U = new double[N_Mem_tot_U];
	double *ae_U = new double[N_Mem_tot_U];
	double *an_U = new double[N_Mem_tot_U];
	double *as_U = new double[N_Mem_tot_U];
	double *ap_U = new double[N_Mem_tot_U];

	memset(aw_U, 0, N_Mem_tot_U * sizeof(double));
	memset(ae_U, 0, N_Mem_tot_U * sizeof(double));
	memset(an_U, 0, N_Mem_tot_U * sizeof(double));
	memset(as_U, 0, N_Mem_tot_U * sizeof(double));
	memset(ap_U, 0, N_Mem_tot_U * sizeof(double));

	double *aw_V = new double[N_Mem_tot_V];
	double *ae_V = new double[N_Mem_tot_V];
	double *an_V = new double[N_Mem_tot_V];
	double *as_V = new double[N_Mem_tot_V];
	double *ap_V = new double[N_Mem_tot_V];

	memset(aw_V, 0, N_Mem_tot_V * sizeof(double));
	memset(ae_V, 0, N_Mem_tot_V * sizeof(double));
	memset(an_V, 0, N_Mem_tot_V * sizeof(double));
	memset(as_V, 0, N_Mem_tot_V * sizeof(double));
	memset(ap_V, 0, N_Mem_tot_V * sizeof(double));

	double *aw_P = new double[N_Mem_tot_P];
	double *ae_P = new double[N_Mem_tot_P];
	double *an_P = new double[N_Mem_tot_P];
	double *as_P = new double[N_Mem_tot_P];
	double *ap_P = new double[N_Mem_tot_P];

	memset(aw_P, 0, N_Mem_tot_P * sizeof(double));
	memset(ae_P, 0, N_Mem_tot_P * sizeof(double));
	memset(an_P, 0, N_Mem_tot_P * sizeof(double));
	memset(as_P, 0, N_Mem_tot_P * sizeof(double));
	memset(ap_P, 0, N_Mem_tot_P * sizeof(double));

	double *F_u = new double[N_Mem_tot_U]; //store the Source-term for u, v, p
	double *F_v = new double[N_Mem_tot_V];

	memset(F_u, 0, N_Mem_tot_U * sizeof(double));
	memset(F_v, 0, N_Mem_tot_V * sizeof(double));

	double *G_u = new double[N_Mem_tot_U];
	double *G_v = new double[N_Mem_tot_V];

	memset(G_u, 0, N_Mem_tot_U * sizeof(double));
	memset(G_v, 0, N_Mem_tot_V * sizeof(double));

	double *G_u_real = new double[N_Mem_tot_U];
	double *G_v_real = new double[N_Mem_tot_V];

	memset(G_u_real, 0, N_Mem_tot_U * sizeof(double));
	memset(G_v_real, 0, N_Mem_tot_V * sizeof(double));

	double *u_real = new double[N_Mem_tot_Central]; //store the real value for u, v, p
	double *v_real = new double[N_Mem_tot_Central];
	double *p_real = new double[N_Mem_tot_Central];

	memset(u_real, 0, N_Mem_tot_Central * sizeof(double));
	memset(v_real, 0, N_Mem_tot_Central * sizeof(double));
	memset(p_real, 0, N_Mem_tot_Central * sizeof(double));

	int iter_start_step = 0;

	//2020-0207-0951 备注开始
	//由于交错网格与原有网格不一致 故控制容积处有区分
	//Set_Grid_And_Constant_Equ_Params 所有参数暂时不用

	//prepost-setgrid and Equ const params
	Set_Grid_And_Constant_Equ_Params(aw_P, ae_P, an_P, as_P, ap_P,
		igrid_x_P, igrid_y_P, GridX_P, GridY_P, dx_h, dy_h);

	Set_Grid_And_Constant_Equ_Params(aw_U, ae_U, an_U, as_U, ap_U,
		igrid_x_U, igrid_y_U, GridX_U, GridY_U, dx_h, dy_h);

	Set_Grid_And_Constant_Equ_Params(aw_V, ae_V, an_V, as_V, ap_V,
		igrid_x_V, igrid_y_V, GridX_V, GridY_V, dx_h, dy_h);

	// ap_UVP may be needed to update during the time

	//2020-0207-0951 备注结束

	//Real-value Load
	Real_Load(u_real, v_real, p_real, GridX_Central, GridY_Central, dx_h, dy_h, iter_start_step);

	Boundary_Load_on_Central(u_c, v_c, u_real, v_real, GridX_Central, GridY_Central);

	//Initializing vectors
	Init_U(u_n, u_real, GridX_U, GridY_U);
	Init_V(v_n, v_real, GridX_V, GridY_V);

	//time-iteration

	for (int iter_t = 0; iter_t < N_t; iter_t++)
	{
		int i_t = iter_t + iter_start_step;

		cout << "-----------------------Next Step----------------------" << endl;
		cout << "TIME_STEP = " << (i_t + 1) << endl;

		/*--------------------------------------G__Update-----------------------------------------------*/
		Get_Source_U(F_u, GridX_U, GridY_U, dx_h, dy_h, i_t);
		Get_Source_V(F_v, GridX_V, GridY_V, dx_h, dy_h, i_t);

		Gu_Update(G_u, F_u, u_n, v_n, GridX_U, GridY_U, dx_h, dy_h);
		Gv_Update(G_v, F_v, u_n, v_n, GridX_V, GridY_V, dx_h, dy_h);

		/*--------------------------------------PossionEqu__Solve-----------------------------------------------*/
		cout << "P_solver iteration " << endl;
		//计算泊松方程的边界条件
		Real_Load(u_real, v_real, p_real, GridX_Central, GridY_Central, dx_h, dy_h, i_t + 1);
		Boundary_Load_on_Central(u_c, v_c, u_real, v_real, GridX_Central, GridY_Central);

		//set ap of Pressure

		//P_solve();

	}

	return 0;
}

