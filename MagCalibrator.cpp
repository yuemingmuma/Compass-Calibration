
#include "MagCalibrator.h"
#include <math.h>
#include <iostream>

//У������������hard iron ��soft iron
calpara_t params;

double sample_buffer[3][COMPASS_CAL_NUM_SAMPLES];
unsigned int samples_collected;


void Initial_CompassCal(calpara_t *params)
{
	  int i;
	  
	  for(i = 0; i < 3; i++)
	  {
	  	  params->offset[i]  = 0.0;
	  	  params->diag[i]    = 1.0;
	  	  params->offdiag[i] = 0.0;
	  }
	  params->radius = 200;/************��Ҫ���ݴ�ǿ��ʵ������˶�************/
	  
	  samples_collected = 0;
	  
}

char accept_sample(double sample[3],double mag_sphere_radius)
{
	  double min_sample_dist,dist;
	  double maxsize = COMPASS_CAL_NUM_SAMPLES;
	  double dx,dy,dz;
	  int i;
	  
	  if(samples_collected==0)
	  {
	  	  return 1;
	  }
	  
	  min_sample_dist = fabs(4.0*mag_sphere_radius/sqrt(maxsize))/3.0;
	  for(i=0;i<samples_collected;i++)
	  {
	      dx = sample[0] - sample_buffer[0][i];
	      dy = sample[1] - sample_buffer[1][i];
	      dz = sample[2] - sample_buffer[2][i];
	      
	      dist = sqrt(dx * dx + dy * dy + dz * dz);
	      
	      if (dist < min_sample_dist)
	      {
	          return 0;
	      }
	  }
	  
	  return 1;
}

void calc_initial_para(calpara_t *params)
{
	  double avr_value,avr_radius;
	  int i,j;
	  
    //��תһȦ����������ݺ�ӦΪ0
    for(i = 0; i < 3; i++)
    {
		avr_value = 0;
        for(j = 0; j < samples_collected; j++) 
        {
            avr_value -= sample_buffer[i][j];
        }
        avr_value /= samples_collected;
        params->offset[i] = avr_value;
    }
    
    //�������������ϴ󣬴˴��ɸ�������ų���С���ù̶�ֵ
    avr_radius = 0;
    for(i = 0; i < samples_collected; i++)
    {
    	  avr_radius += sqrt(sample_buffer[0][i]*sample_buffer[0][i]+
    	                     sample_buffer[1][i]*sample_buffer[1][i]+
    	                     sample_buffer[2][i]*sample_buffer[2][i]);
    }
    avr_radius /= samples_collected;
    params->radius = avr_radius;
} 

double calc_mean_residual(const double x[], const double y[], const double z[], unsigned int size,                
						double offset_x, double offset_y, double offset_z,
						double sphere_radius, double diag_x, double diag_y, double diag_z, 
						double offdiag_x, double offdiag_y, double offdiag_z)
{
	int k;
	double A,B,C;
	double fitvalue,length,residual;

	fitvalue = 0;
	for ( k = 0; k < size; k++) 
	{
		//Calculate Jacobian
		A = (diag_x    * (x[k] + offset_x)) + (offdiag_x * (y[k] + offset_y)) + (offdiag_y * (z[k] + offset_z));
		B = (offdiag_x * (x[k] + offset_x)) + (diag_y    * (y[k] + offset_y)) + (offdiag_z * (z[k] + offset_z));
		C = (offdiag_y * (x[k] + offset_x)) + (offdiag_z * (y[k] + offset_y)) + (diag_z    * (z[k] + offset_z));
		length = sqrt(A * A + B * B + C * C);
		residual = sphere_radius - length;//sphere_radius Ϊ��ֵ

		fitvalue += residual*residual;
	}
	fitvalue = sqrt(fitvalue / size);

	return fitvalue;
}

char InverseMatrix(int dimension, double  matrix[])
{
    int column_swap[20];
    int l,k,m;
    double  pivot;
    double  determinate;
    double  swap;

    determinate = 1.0;

    for(l=0;l<dimension;l++)
    {
        column_swap[l] = l;
    }

    for(l=0;l<dimension;l++)
    {
        pivot = 0.0;
        m = l;

        /* Find the element in this row with the largest absolute value -
           the pivot. Pivoting helps avoid division by small quantities. */
        for(k=l;k<dimension;k++)
        {
            if(fabs(pivot) < fabs(matrix[dimension*l+k]))
            {
                m = k;
                pivot = matrix[dimension*l+k];
            }
        }

        /* Swap columns if necessary to make the pivot be the leading
           nonzero element of the row. */
        if(l!=m)
        {
            k = column_swap[m];
            column_swap[m] = column_swap[l];
            column_swap[l] = k;
            for(k=0;k<dimension;k++)
            {
                swap = matrix[dimension*k+l];
                matrix[dimension*k+l] = matrix[dimension*k+m];
                matrix[dimension*k+m] = swap;
            }
        }

        /* Divide the row by the pivot, making the leading element
           1.0 and multiplying the determinant by the pivot. */
        matrix[dimension*l+l] = 1.0;
        determinate = determinate * pivot;   /* Determinant of the matrix. */
        if(fabs(determinate) < 1.0E-13)
        {
            return 0;      /* Pivot = 0 therefore singular matrix. */
        }
        for(m=0;m<dimension;m++)
        {
            matrix[dimension*l+m] = matrix[dimension*l+m] / pivot;
        }

        /* Gauss-Jordan elimination.  Subtract the appropriate multiple
           of the current row from all subsequent rows to get the matrix
           into row echelon form. */

        for(m=0;m<dimension;m++)
        {
            if(l==m)
            {
                continue;
            }
            pivot = matrix[dimension*m+l];

            matrix[dimension*m+l] = 0.0;
            for(k=0;k<dimension;k++)
            {
                matrix[dimension*m+k] = matrix[dimension*m+k] - (pivot*matrix[dimension*l+k]);
            }
        }
    }

    /* Swap the columns back into their correct places. */
    for(l=0;l<dimension;l++)
    {
        if(column_swap[l]==l)
        {
            continue;
        }

        /* Find out where the column wound up after column swapping. */
        for(m=l+1;m<dimension;m++)
        {
            if(column_swap[m]==l)
            {
                break;
            }
        }

        column_swap[m] = column_swap[l];
        for(k=0;k<dimension;k++)
        {
            pivot = matrix[dimension*l+k];
            matrix[dimension*l+k] = matrix[dimension*m+k];
            matrix[dimension*m+k] = pivot;
        }
        column_swap[l] = l;
    }
    return 1;
}

char run_gn_sphere_fit(const double x[], const double y[], const double z[], unsigned int size,                
                     double *offset_x, double *offset_y, double *offset_z,
		                 double *sphere_radius, double *diag_x, double *diag_y, double *diag_z, 
		                 double *offdiag_x, double *offdiag_y, double *offdiag_z)
{
    double sphere_jacob[4];
	double JTJ[16],JTFI[4];
	double residual = 0.0,fitness;
	double A,B,C,length;
	double deltax[4],deltavalue;
	int i,j,k,row,col;

	//Initial zeros
	for ( i = 0; i < 4; i++)
	{
		for ( j = 0; j < 4; j++)
		{
			JTJ[i * 4 + j] = 0;
		}
		JTFI[i] = 0;
	}
	
	// Gauss Newton 
	for ( k = 0; k < size; k++) 
	{
		//Calculate Jacobian
		A = (*diag_x    * (x[k] + *offset_x)) + (*offdiag_x * (y[k] + *offset_y)) + (*offdiag_y * (z[k] + *offset_z));
		B = (*offdiag_x * (x[k] + *offset_x)) + (*diag_y    * (y[k] + *offset_y)) + (*offdiag_z * (z[k] + *offset_z));
		C = (*offdiag_y * (x[k] + *offset_x)) + (*offdiag_z * (y[k] + *offset_y)) + (*diag_z    * (z[k] + *offset_z));
		length = sqrt(A * A + B * B + C * C);
		
		// 0: �԰뾶����
		sphere_jacob[0] = 1.0;
		// 1-3: ��bias x��y��z����
		sphere_jacob[1] = -1.0 * (((*diag_x    * A) + (*offdiag_x * B) + (*offdiag_y * C)) / length);
		sphere_jacob[2] = -1.0 * (((*offdiag_x * A) + (*diag_y    * B) + (*offdiag_z * C)) / length);
		sphere_jacob[3] = -1.0 * (((*offdiag_y * A) + (*offdiag_z * B) + (*diag_z    * C)) / length);
		residual = *sphere_radius - length;//sphere_radius Ϊ��ֵ
		
		for (i = 0; i < 4; i++)
		{
				// compute JTJ
				for (j = 0; j < 4; j++)
				{
					JTJ[i * 4 + j] += sphere_jacob[i] * sphere_jacob[j];
				}
			
				JTFI[i] += sphere_jacob[i] * residual;
		}
	}
	
	double fit_params[4] = {*sphere_radius, *offset_x, *offset_y, *offset_z};
	
	if (!InverseMatrix(4,JTJ)) 
	{
			std::cout<<"Mat can not inverse!!"<<std::endl;
			return 0;
	}
	
	deltavalue = 0;
	for (row = 0; row < 4; row++)
	{
		deltax[row] = 0;
		for (col = 0; col < 4; col++) 
		{
			deltax[row] -= JTFI[col] * JTJ[row * 4+ col];
		}
		deltavalue += deltax[row]*deltax[row];

		fit_params[row] += deltax[row];
	}

	deltavalue = sqrt(deltavalue);

	fitness = calc_mean_residual(x,y,z,size,*offset_x,*offset_y,*offset_z,*sphere_radius,*diag_x,*diag_y,*diag_z,*offdiag_x,*offdiag_y,*offdiag_z);
	std::cout<<"The objective value = "<<fitness<<", increment = "<<deltavalue<<std::endl;

	*sphere_radius = fit_params[0];
	*offset_x = fit_params[1];
	*offset_y = fit_params[2];
    *offset_z = fit_params[3];	

	if (deltavalue < 1e-5)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

char run_gn_ellipsoid_fit(const double x[], const double y[], const double z[], unsigned int size,
                          double *offset_x, double *offset_y, double *offset_z,
			                    double *sphere_radius, double *diag_x, double *diag_y, double *diag_z, 
			                    double *offdiag_x, double *offdiag_y, double *offdiag_z)
{
    double JTJ[81],JTFI[9];
	double ellipsoid_jacob[9];
	double fitness;
	double A,B,C,length,residual;
	double deltax[9],deltavalue;
	int i,j,k,row,col;

	//Initial zeros
	for ( i = 0; i < 9; i++)
	{
		for ( j = 0; j < 9; j++)
		{
			JTJ[i * 9 + j] = 0;
		}
		JTFI[i] = 0;
	}
	
	// Gauss Newton
	for (k = 0; k < size; k++) 
	{
	
		//Calculate Jacobian
		A = (*diag_x    * (x[k] + *offset_x)) + (*offdiag_x * (y[k] + *offset_y)) + (*offdiag_y * (z[k] + *offset_z));
		B = (*offdiag_x * (x[k] + *offset_x)) + (*diag_y    * (y[k] + *offset_y)) + (*offdiag_z * (z[k] + *offset_z));
		C = (*offdiag_y * (x[k] + *offset_x)) + (*offdiag_z * (y[k] + *offset_y)) + (*diag_z    * (z[k] + *offset_z));
		length = sqrt(A * A + B * B + C * C);
		residual = *sphere_radius - length;
				
		// 0-2: ��bias x��y��z����
		ellipsoid_jacob[0] = -1.0 * (((*diag_x    * A) + (*offdiag_x * B) + (*offdiag_y * C)) / length);
		ellipsoid_jacob[1] = -1.0 * (((*offdiag_x * A) + (*diag_y    * B) + (*offdiag_z * C)) / length);
		ellipsoid_jacob[2] = -1.0 * (((*offdiag_y * A) + (*offdiag_z * B) + (*diag_z    * C)) / length);
		// 3-5: ��diag x��y��z����
		ellipsoid_jacob[3] = -1.0 * ((x[k] + *offset_x) * A) / length;
		ellipsoid_jacob[4] = -1.0 * ((y[k] + *offset_y) * B) / length;
		ellipsoid_jacob[5] = -1.0 * ((z[k] + *offset_z) * C) / length;
		// 6-8: ��offdiag x��y��z����
		ellipsoid_jacob[6] = -1.0 * (((y[k] + *offset_y) * A) + ((x[k] + *offset_x) * B)) / length;
		ellipsoid_jacob[7] = -1.0 * (((z[k] + *offset_z) * A) + ((x[k] + *offset_x) * C)) / length;
		ellipsoid_jacob[8] = -1.0 * (((z[k] + *offset_z) * B) + ((y[k] + *offset_y) * C)) / length;
		
		for (i = 0; i < 9; i++)
		{
		    // compute JTJ
			for (j = 0; j < 9; j++)
			{
				JTJ[i * 9 + j] += ellipsoid_jacob[i] * ellipsoid_jacob[j];
			}
			
			JTFI[i] += ellipsoid_jacob[i] * residual;
		}
	}
	
    double fit_params[9] = {*offset_x, *offset_y, *offset_z, *diag_x, *diag_y, *diag_z, *offdiag_x, *offdiag_y, *offdiag_z};
	
	if (!InverseMatrix(9,JTJ))
	{
		return 0;
	}
	
	deltavalue = 0;
	for (row = 0; row < 9; row++)
	{
		deltax[row] = 0;
	    for (col = 0; col < 9; col++)
		{
			deltax[row] -= JTFI[col] * JTJ[row * 9 + col];
		}
		deltavalue += deltax[row]*deltax[row];

		fit_params[row] +=  deltax[row];
	}
	deltavalue = sqrt(deltavalue);

	fitness = calc_mean_residual(x,y,z,size,*offset_x,*offset_y,*offset_z,*sphere_radius,*diag_x,*diag_y,*diag_z,*offdiag_x,*offdiag_y,*offdiag_z);
	std::cout<<"The objective value = "<<fitness<<", increment = "<<deltavalue<<std::endl;

	*offset_x = fit_params[0];
	*offset_y = fit_params[1];
	*offset_z = fit_params[2];
	*diag_x = fit_params[3];
	*diag_y = fit_params[4];
	*diag_z = fit_params[5];
	*offdiag_x = fit_params[6];
	*offdiag_y = fit_params[7];
	*offdiag_z = fit_params[8];

	if (deltavalue < 1e-5)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void ellipsoid_fit_least_squares(double x[],double y[],double z[],unsigned int size,calpara_t *params,double *finalfitness)
{
	  double max_iterations = 100;
	  double offset_x = params->offset[0],offset_y = params->offset[1],offset_z = params->offset[2];
	  double diag_x   =  params->diag[0] ,diag_y   =  params->diag[1] ,diag_z   =  params->diag[2] ;
	  double offdiag_x=params->offdiag[0],offdiag_y=params->offdiag[1],offdiag_z=params->offdiag[2];
	  double sphere_radius = params->radius;
	  char stopflag;
	  int i;
	 
	  //���ȶ԰뾶��bias�����Ż� 
	  std::cout<<"/*********************run_gn_sphere_fit****************************/"<<std::endl;
	  for (i = 0; i < max_iterations; i++) 
	  {
		    stopflag = run_gn_sphere_fit(x, y, z, size, &offset_x, &offset_y, &offset_z,
				                         &sphere_radius, &diag_x, &diag_y, &diag_z, &offdiag_x, &offdiag_y, &offdiag_z);

			if (stopflag ==1)
			{
				break;
			}
	  }
	
	  //Ȼ��Գ��뾶�������9�����������Ż�
	  std::cout<<"/*********************run_gn_ellipsoid_fit****************************/"<<std::endl;
	  for (i = 0; i < max_iterations; i++)
	  {
		    stopflag = run_gn_ellipsoid_fit(x, y, z, size, &offset_x, &offset_y, &offset_z,
				                            &sphere_radius, &diag_x, &diag_y, &diag_z, &offdiag_x, &offdiag_y, &offdiag_z);

			if (stopflag ==1)
			{
				break;
			}
	  }

	  std::cout<<"/*********************fit end****************************/"<<std::endl;
	  *finalfitness = calc_mean_residual(x,y,z,size,offset_x,offset_y,offset_z,sphere_radius,diag_x,diag_y,diag_z,offdiag_x,offdiag_y,offdiag_z);
	  std::cout<<"Final fitness = "<<*finalfitness<<std::endl;

	  //�û���ȥ
	  params->offset[0] = offset_x;params->offset[1] = offset_y;params->offset[2] = offset_z;
	  params->diag[0]   =   diag_x;params->diag[1]   =   diag_y;params->diag[2]   =   diag_z;
	  params->offdiag[0]=offdiag_x;params->offdiag[1]=offdiag_y;params->offdiag[2]=offdiag_z;
	  params->radius = sphere_radius;
}

char check_calibration_result(calpara_t params,double fitness)
{
	  double tolerance = 5.0;
	  
	  //The maximum measurement range is ~1.9 Ga, the earth field is ~0.6 Ga, so an offset larger than ~1.3 Ga means the mag will saturate in some directions.
	  double offset_max = 1300;	//mG
	
	  
	  if( (fitness <= tolerance ) &&
        (params.radius > 150) && (params.radius < 950) && //Earth's magnetic field strength range: 250-850mG
        (fabs(params.offset[0]) < offset_max) &&
        (fabs(params.offset[1]) < offset_max) &&
        (fabs(params.offset[2]) < offset_max) &&
        (params.diag[0] > 0.2) && (params.diag[0] < 5.0) &&
        (params.diag[1] > 0.2) && (params.diag[1] < 5.0) &&
        (params.diag[2] > 0.2) && (params.diag[2] < 5.0) &&
        (fabs(params.offdiag[0]) <  1.0) &&      //absolute of sine/cosine output cannot be greater than 1
        (fabs(params.offdiag[1]) <  1.0) &&
        (fabs(params.offdiag[2]) <  1.0) )
	  {

            return 1;
      }
	  else
	  {
		    return 2;
	  }
}

//��ǿ��У��������
//���룺sample   ˮƽ��ת�����е�ĳ����������
//������־��0-У���У�1-У������,�ɹ���2-У������,ʧ��
char CompassCal(double sample[3])
{
	  static char cal_state = 0;
	  static double mag_sphere_radius = 0.0;
	  
	  char new_sample = 0,calresult = 0;
	  double fitness;
	  int i;
	  
	  //��ʼ��
	  if(cal_state==0)
	  {
	      Initial_CompassCal(&params);
	      mag_sphere_radius = sqrt(sample[0]*sample[0] + sample[1]*sample[1] + sample[2]*sample[2]);
	      
	      cal_state++;
	  }
	  
	  //����
	  if(cal_state==1)
	  {
	  	  new_sample  = accept_sample(sample,mag_sphere_radius);
	  	  if(new_sample == 1)
	  	  {
	  	  	  for(i=0;i<3;i++)
	  	  	  {
	  	  	      sample_buffer[i][samples_collected] = sample[i];
	  	  	  }
	  	  	  samples_collected++;
			  
			  //////////////////////
			  std::cout<<"samples_collected = "<<samples_collected<<std::endl;

	  	  	  if(samples_collected == COMPASS_CAL_NUM_SAMPLES)
	  	  	  {
	  	  	  	  cal_state++;
	  	  	  }
	  	  }
	  }
	  
	  //����Ż�
	  if(cal_state==2)
	  {
	  	  //�Ż���ʼֵ
	  	  calc_initial_para(&params);
	  	  //�Ż�
	  	  ellipsoid_fit_least_squares(sample_buffer[0],sample_buffer[1],sample_buffer[2],samples_collected,&params,&fitness);
	  	  //�����
	  	  calresult = check_calibration_result(params,fitness);
	  }

	  return calresult;  
}