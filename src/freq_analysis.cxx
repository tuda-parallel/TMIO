
#include "freq_analysis.h"

namespace freq_analysis
{

	//**********************************************************************
	//*                       1. Dft
	//**********************************************************************
	/**
	 * @brief * @brief Computes the discrete fourier transformation
	 *
	 * @param b bandwidth vector
	 * @param t time vector
	 * @param n number of element in t or b
	 * @param freq sample frequency
	 * @return std::complex<double>*
	 */
#if DFT >= 1
	std::complex<double> *Dft(double *b, double *t, int n, bool flag_req, bool w_or_r, int procs, double& dft_time, double freq)
	{
		// few references:
		//(1) https://pythonnumericalmethods.berkeley.edu/notebooks/chapter24.02-Discrete-Fourier-Transform.html
		//(2) https://www.robots.ox.ac.uk/~sjrob/Teaching/SP/l7.pdf

		// compute DFT
		double t_over = MPI_Wtime();
		MPI_File fh, fh2;

		int size = 150;
		char buf[size];
		bool verbose = false;
		bool verbose_time = false;
		double time_tmp = t_over;
		bool discret_error = true;
		//? for DFT:
    	double dominant_freq = -1; // saves frequency for repeated times
	    int hits = 0;        // count how many times the frequency was correctly
    	double t_short = -1;       // shorten time window if DFT_TIME_WINDOW is active. Else leave value at -1
		MPI_Request req;
		std::string name = "sync";
		std::string s = "";
		std::string s2 = "";
		std::complex<double> *X = NULL;

		if (flag_req)
			name = "async";
		if (w_or_r)
			name.append("_write");
		else
			name.append("_read");

		// create files
		snprintf(buf, size, "%d_DFT.json", procs);
		if (flag_req && w_or_r) // first run (async write)
			MPI_File_delete(buf, MPI_INFO_NULL);
		MPI_File_open(MPI_COMM_SELF, buf, MPI_MODE_CREATE | MPI_MODE_WRONLY | MPI_MODE_APPEND, MPI_INFO_NULL, &fh);

		snprintf(buf, size, "%d_%s.jsonl", procs, name.c_str());
		if (flag_req && w_or_r) // first run (async write)
			MPI_File_delete(buf, MPI_INFO_NULL);
		MPI_File_open(MPI_COMM_SELF, buf, MPI_MODE_CREATE | MPI_MODE_WRONLY | MPI_MODE_APPEND, MPI_INFO_NULL, &fh2);

		// write name of mode
		// name = "#! " + name + "\n# ---------------------";
		std::cout << "\nDFT for " << name << ":\n******************\n";

		name = "\"" + name + "\":{\n\t\"data\":{\n";
		if (flag_req and w_or_r)
			name = "{\n\n" + name;
		MPI_File_write(fh, name.c_str(), strlen(name.c_str()), MPI_CHAR, MPI_STATUS_IGNORE);

		if (n != 0)
		{
			// recommend freq:
			double t_rec = std::numeric_limits<double>::infinity();
			for (int i = 0; i < n - 1; i++)
			{
				if (t_rec > (t[i + 1] - t[i]) && (t[i + 1] - t[i]) >= 0.001)
					t_rec = t[i + 1] - t[i];
			}
			std::cout << "Recommeded sampling frequency is " << BLUE << 2 / t_rec << "Hz " << BLACK << "(T = " << t_rec << "*2 sec)" << std::endl;
			if (verbose_time)
			{
				time_tmp = MPI_Wtime() - time_tmp;
				std::cout << RED << " > Consumed DFT time so far: " << time_tmp << BLACK << std::endl;
				time_tmp = MPI_Wtime();
			}
			// assign freq
			if (freq < 0)
			{
				if (2 / t_rec > FREQ_LIMIT)
					freq = FREQ_LIMIT;
				else
				{
					freq = 2 / t_rec;	   // limit maximal sampling frequency to 100Hz
					discret_error = false; // there will be no discretization error if high sampling rate is used.
				}
				std::cout << "> Auto set sampling frequency to " << BLUE << freq << "Hz " << BLACK << std::endl;
			}
			else
				std::cout << "> User set sampling frequency to " << RED << freq << "Hz " << BLACK << std::endl;

			int N = floor((t[n - 1] - t[0]) * freq);
			snprintf(buf, size, "\n N -> %d, (t[n-1] -t[0]) -> %f, freq -> %f, (t[n-1] -t[0]) * freq = %f\n", N, (t[n - 1] - t[0]), freq, (t[n - 1] - t[0]) * freq);
			if (verbose)
				std::cout << buf;
			// MPI_File_write(fh, buf, strlen(buf), MPI_CHAR, MPI_STATUS_IGNORE);
			freq = (double)N / (t[n - 1] - t[0]);
			double Ts = freq > 0 ? 1 / freq : 0;

			snprintf(buf, size, "# Total time: %.6f - %.6f = %.6f --> sample frequency = %f  --> total samples = %d\n", t[n - 1], t[0], t[n - 1] - t[0], freq, N);
			if (verbose)
				std::cout << buf;
			// MPI_File_write(fh, buf, strlen(buf), MPI_CHAR, MPI_STATUS_IGNORE);

			X = (std::complex<double> *)malloc(sizeof(std::complex<double>) * N);
			double b_sampled[N], A[N], phi[N], tot_A;
			//! sample with freq N samples starting at t_short:
			DFT_Sample(N, freq, b_sampled, t, b, n, t_short);
			if (verbose_time)
			{
				time_tmp = MPI_Wtime() - time_tmp;
				std::cout << RED << " > Consumed DFT time after sampling: " << time_tmp << BLACK << std::endl;
				time_tmp = MPI_Wtime();
			}
			//! perform DFT
			DFT_Core(N, b_sampled, X, A, phi, tot_A);
			if (verbose_time)
			{
				time_tmp = MPI_Wtime() - time_tmp;
				std::cout << RED << " > Consumed DFT after Core function: " << time_tmp << BLACK << std::endl;
				time_tmp = MPI_Wtime();
			}

			//? check how good discretization is: cacluate Area of both an d compute realitve deviation
			double A_o = 0;
			double A_r = 0;
			for (int i = 0; i < n; i++)
				if (!isnan(b[i]))
					A_o += b[i] * (t[i + 1] - t[i]);

			for (int i = 0; i < N; i++)
				A_r += b_sampled[i] * (1 / freq);
			if (discret_error == true)
			{
				double error = (A_o > 0) ? std::abs(A_o - A_r) / A_o : 0;
				std ::cout << YELLOW << "Relative abstraction error is -> " << error << BLACK << std::endl;
				if (verbose_time)
				{
					time_tmp = MPI_Wtime() - time_tmp;
					std::cout << RED << " > Consumed DFT after abstraction error: " << time_tmp << BLACK << std::endl;
					time_tmp = MPI_Wtime();
				}
			}
			// std::cout << N << " sampled values" << std::endl;

			//? Z-score to determine relevance
			double mean = iohf::Arithmetic_Mean(A + 1, N - 1); // N - 1);
			double var = iohf::Standard_Deviation(A + 1, N - 1, mean);
			// std::cout << "Mean is:" << mean << "   Standard deviation is" << var << std::endl;
			double Z[N];
			double tot_Z = 1;
			for (int k = 0; k < N; k++)
			{
				if (var != 0)
					Z[k] = abs(A[k] - mean) / var;
				//  std::cout << "Z-Score for k = " << k << " (freq = "<< k/(N*Ts)<< ") is " << (A[k] - mean)/var << std::endl;
				else
					Z[k] = 0;

				if (verbose && Z[k] > 3 && k > 0 && k < N / 2 + 1 && (N * Ts) / k > 0.1) // only count if outside of one standard deviation also look only of time greater than 0.1 sec
					tot_Z += Z[k];
			}
			if (verbose_time)
			{
				time_tmp = MPI_Wtime() - time_tmp;
				std::cout << RED << " > Consumed DFT after Z-Score: " << time_tmp << BLACK << std::endl;
				time_tmp = MPI_Wtime();
			}

			// first value is dc offset. Since the signal is not centered arroun 0, the value is alawys max
			//? find dominant frequency
			double max = iohf::Max(Z + 1, N / 2 + 1);
			double dominant_Z_tot = 0;
			int dominant_counter = 0;
			double confidence = 0;
			// std::vector<double> dominant_frequencies;
			for (int k = 0; k < N / 2; k++)
			{
				if (Z[k] > .8 * max && k != 0) // only count if outside of one standard deviation also look only of time greater than 0.1 sec
				{
					dominant_Z_tot += Z[k];
					dominant_counter++;
				}
			}

			if (dominant_counter <= 3)
			{
				for (int k = 0; k < N / 2; k++)
				{
					if (Z[k] > .8 * max && k != 0 && Z[k] / dominant_Z_tot > .2)
					{
						if (Z[k] == max)
						{
//! make confidence check for dominant frequency
#if CONFIDENCE_CHECK > 0
							printf("%sk = %i  -- N = %i -- Ts = %.3f -- freq = %.3f\n", BLUE, k, N, Ts, freq);
							confidence = DFT_Confidence_Check(k / (N * Ts), freq, N, b_sampled);
#endif
							std::cout << GREEN << "Expected perodicity: " << 1 / (k / (N * Ts)) << " sec"
									  << " (frequency bin: " << k << ", frequency: " << k / (N * Ts) << " Hz) --> " << RED << Z[k] / dominant_Z_tot * 100 << "% dominant"
									  << " --> confidence " << confidence << "%" << BLACK << std::endl;
//? decrease time window after several hits
#if DFT_TIME_WINDOW > 0
							if (N != 0 || k != 0 || Ts != 0)
							{

								double f = k / (N * Ts);
								double tol = freq / N; // *80/100;
								if (dominant_freq == -1 || (dominant_freq > f - tol && dominant_freq < f + tol))
									hits += 1;
								else
								{
									hits = 0;
									dominant_freq = -1;
									printf("%s Reset prediction%s\n", BLUE, BLACK);
								}
								if (hits >= 1)
								{
									dominant_freq = f;
									printf("%shifts dominant frequency %.3f Hz (tollerance %.2f Hz) is %i %s\n", BLUE, dominant_freq, tol, hits, BLACK);
									if (hits > 2)
									{
										t_short = t[n - 1] - 2 * 1 / f;
										printf("%sStart time changed to %.3f %s\n", BLUE, t_short, BLACK);
									}
								}
								else
									t_short = -1;
							}
#endif
						}
						else
						{
							std::cout << GREEN << "Expected perodicity: " << 1 / (k / (N * Ts)) << " sec"
									  << " (frequency bin: " << k << ", frequency: " << k / (N * Ts) << " Hz) --> " << RED << Z[k] / dominant_Z_tot * 100 << "% dominant" << BLACK << std::endl;
						}
						// dominant_frequencies.push_back(k / (N * Ts));
					}
				}
			}
			else
				std::cout << YELLOW << "no dominant frequency (signal might be non-preodic)" << BLACK << std::endl;

			if (verbose_time)
			{
				time_tmp = MPI_Wtime() - time_tmp;
				std::cout << RED << " > Consumed DFT after dominant freq: " << time_tmp << BLACK << std::endl;
				time_tmp = MPI_Wtime();
			}

			//? Print results
			// double max = iohf::Max(A + 1, N/2 + 1);
			bool all = true;
			if (all)
			{
				for (int k = 0; k < N; k++)
				{
					if (verbose)
					{
						std::string color = (Z[k] >= 0.98 * max && Z[k] >= 2) ? RED : (Z[k] > 0.4 * max) ? YELLOW
																				  : (Z[k] > 0.1 * max)	 ? BLUE
																										 : BLACK;
						std::cout << "X[" << k << "] = " << X[k] << " --> " << color << Z[k] << " -- " << Z[k] / tot_Z * 100 << "% domiant" << BLACK << std::endl;
						if (color == RED && k < N / 2 + 1 && k != 0)
						{
							// if (color == RED && k != 0 && k < N/2 +1)
							std::cout << GREEN << "Expected perodicity: " << 1 / (k / (N * Ts)) << " sec"
									  << " (frequency bin: " << k << ", frequency: " << k / (N * Ts) << " Hz)" << BLACK << std::endl;
						}
					}
					snprintf(buf, size, "{\"params\":{\"x\":%d,\"y\":%d},\"callpath\":\"%s->freq\",\"metric\":\"amplitude\",\"value\": %.2e }\n", procs, k, name.c_str(), A[k]);
					s.append(buf);
				}

				MPI_File_iwrite(fh2, s.c_str(), strlen(s.c_str()), MPI_CHAR, &req);
				s2 = DFT_Create_String("A", A, N, buf, size);
				MPI_Wait(&req, MPI_STATUS_IGNORE);
				MPI_File_iwrite(fh, s2.c_str(), strlen(s2.c_str()), MPI_CHAR, &req);
				s = DFT_Create_String("phi", phi, N, buf, size);
				MPI_Wait(&req, MPI_STATUS_IGNORE);
				MPI_File_iwrite(fh, s.c_str(), strlen(s.c_str()), MPI_CHAR, &req);
			}
			else
			{
				for (int k = 0; k <= ceil(N / 2); k++)
				{
					std::string color = (A[k] >= max) ? RED : (A[k] > 0.5 * max) ? YELLOW
														  : (A[k] > 0.1 * max)	 ? BLUE
																				 : BLACK;

					if (k != 0 && k != ceil(N / 2))
					{
						std::cout << "X[" << k << "] = " << X[k] << " --> Amp:" << color << 2 * A[k] << BLACK << "   angle: " << color << phi[k] << BLACK << std::endl;
					}
					else
					{
						std::cout << "X[" << k << "] = " << X[k] << " --> Amp:" << color << A[k] << BLACK << "   angle: " << color << phi[k] << BLACK << std::endl;
					}
				}

				for (int k = 0; k <= ceil(N / 2); k++)
				{
					if (k == 0)
						std::cout << "A = [" << A[k] << ",";
					else if (k == ceil(N / 2))
						std::cout << A[k] << "]\n";
					else
						std::cout << 2 * A[k] << ",";
				}

				for (int k = 0; k <= ceil(N / 2); k++)
				{
					if (k == 0)
						std::cout << "phi = [" << phi[k] << ",";
					else if (k == ceil(N / 2))
						std::cout << phi[k] << "]\n";
					else
						std::cout << phi[k] << ",";
				}
			}

			// s2 = DFT_Create_String("b_sampled",b_sampled, counter, buf, size, true);
			s2 = DFT_Create_String("b_sampled", b_sampled, N, buf, size, true);
			MPI_Wait(&req, MPI_STATUS_IGNORE);
			MPI_File_write(fh, s2.c_str(), strlen(s2.c_str()), MPI_CHAR, MPI_STATUS_IGNORE);

			// todo: add flag to control
#if DFT > 1
			s2 = "\t},\n\t\"original\":{\n" + DFT_Create_String("b", b, n, buf, size);
			MPI_File_write(fh, s2.c_str(), strlen(s2.c_str()), MPI_CHAR, MPI_STATUS_IGNORE);
			s2 = DFT_Create_String("t", t, n, buf, size, true);
			MPI_File_write(fh, s2.c_str(), strlen(s2.c_str()), MPI_CHAR, MPI_STATUS_IGNORE);
#endif

			snprintf(buf, size, "\t\"t_start\" : %f,\n\t\"t_end\"   : %f,\n\t\"T_s\"     : %f,\n\t\"N\"       : %i,\n\t\"ranks\"   : %i\n", t[0], t[n - 1], Ts, N, procs);
			s = "\t},\n\t\"settings\":{\n";
			s.append(buf);
			if (verbose)
				std::cout << buf;
			MPI_File_write(fh, s.c_str(), strlen(s.c_str()), MPI_CHAR, MPI_STATUS_IGNORE);

			// find DFT overhead time

			if (verbose_time)
				std::cout << RED << " > Consumed DFT time: " << MPI_Wtime() - t_over << BLACK << std::endl;
			dft_time = MPI_Wtime() - t_over;
		}

		s = "\t}}";
		if (!flag_req && !w_or_r)
			s = s + "\n\n}\n\n";
		else
			s = s + ",\n\n";
		MPI_File_write(fh, s.c_str(), strlen(s.c_str()), MPI_CHAR, MPI_STATUS_IGNORE);
		MPI_File_close(&fh);
		MPI_File_close(&fh2);

		return X;
	}

	//**********************************************************************
	//*                       2. DFT_Sample
	//**********************************************************************
	/**
	 * @brief samples a given signal with y values b and x values t at 1/freq time intervals
	 *
	 * @param N [in] Total number of desired samples. This is calulated with the sampling freq: N = floor((t[n - 1] - t[0]) * freq).
	 *          Garunties no alliasing occurs, as the freq is specified so that N end at the last data point
	 * @param freq [in] Sampling frequency. used here to specify the time steps
	 * @param b_sampled [out] The sapled bandwidth at the timesteps 1/freq
	 * @param t [in] array of time. At each time instance, @see b attains a new value
	 * @param b [i] array of bandwidth corresponding to t
	 * @param n length of @see b or t arrays
	 */
	void DFT_Sample(int N, double freq, double *b_sampled, double *t, double *b, int n, double t_short)
	{
		//? find samples: sample b with sample rate 1/freq to get b_sampled

		int counter = 0;
		int n_old = 0;
		// shorten time internval
		if (t_short < t[0])
			t_short = t[0];
		else
			printf("%sSampling starting at %.3f \n", BLUE, t_short);

		for (double t_sample = t_short; counter < N; t_sample += 1 / freq)
		{
			// std::cout << "t_sample: " << t_sample << "\n";
			for (int i = n_old; i < n; i++)
			{
				// std :: cout << "t[i] -- t[i+1] --> " << t[i] << " -- " << t[i+1] << std::endl;
				if (((t_sample >= t[i]) && (t_sample < t[i + 1])) || i == n - 1)
				{
					// std :: cout << GREEN << "found -> " << b[i] << BLACK << std::endl;
					n_old = i; // no need to itterte over entire array
					if (!isnan(b[i]))
						b_sampled[counter++] = b[i];
					else
						b_sampled[counter++] = 0;
					break;
				}
			}
		}
	}

	//**********************************************************************
	//*                       3. DFT_Core
	//**********************************************************************
	/**
	 * @brief performs DFT
	 *
	 * @param N [in] Total number of samples (sampled with freq over time interval [t0,t[n-1]]
	 * @param b_sampled [in] The sample bandwidth with freq. Obtained via @func DFT_Sample
	 * @param X [out] Complex DFT signal
	 * @param A [out] Amplitude of X
	 * @param phi [out] Phase of X
	 * @param tot_A [out] Sum of amplitudes of X
	 */
	void DFT_Core(int N, double *b_sampled, std::complex<double> *X, double *A, double *phi, double &tot_A)
	{

		using namespace std::complex_literals;
		tot_A = 0;
		for (int k = 0; k < N; k++)
			X[k] = 0i;

		const double PI = std::acos(-1);
		//? perform DFT
		for (int k = 0; k < N; k++)
		{
			for (int n = 0; n < N; n++)
			{

				X[k] += b_sampled[n] * std::exp((-2 * PI * n * k / N) * 1i);
				// std::cout << "X[" << k << "] = " << X[k] << std::endl;
			}
			//? calculate amplitude, phase, and sum of amplitudes

			A[k] = std::abs(X[k]);
			phi[k] = std::atan2(imag(X[k]), real(X[k])); // rad
			tot_A += A[k];
		}
	}

	//**********************************************************************
	//*                       4. DFT_Confidence_Check
	//**********************************************************************
	/**
	 * @brief checks that the dominant frequncy is presented in chunks of the signal.
	 *  The signal is first devided into equal sized chunks. The chunks have a length of 1/f_dominant.
	 * Due to dividing the the signal in equal space chunks, aliasing occurs (frequency steps change) which is why the freuqncy step changes (N stays constant!)
	 * the uncertatiny is considered in the examination of the domminant freuqency.
	 *
	 * @param dominant_freq [in] Dominant frequency of the signal
	 * @param freq [in] Sampling frequency
	 * @param N [in] total number of samples
	 * @param b_sampled [in]
	 * @return [out, double] confidence in the dominant frequency
	 */

	// void DFT_Confidence_Check(std::vector<double> dominant_freqs, double freq, int N, double* b_sampled)
	double DFT_Confidence_Check(double dominant_freq, double freq, int N, double *b_sampled)
	{
		double confidence = 0;
		double Total_time_window = 1 / freq * N;
		int N_check = Total_time_window * dominant_freq;
		double max = 0;
		int index = 0;
		double check_freq;
		// double confidence_interval = N / (Total_time_window * dominant_freq) - N_subset;
		// std::cout << "sampling alias: " << confidence_interval << std::endl;
		// confidence_interval = confidence_interval/ (N_subset * 1 / freq);
		if (N_check == 0 || freq == 0 || N_check > 10)
		{ /// N_subset not more than 10 splits alloweded
			std::cout << BLUE << "Confidence check skipped: N_check = " << N_check << "  freq = " << freq << BLACK << std::endl;
			return 0;
		}
		int N_subset = N / N_check;
		std::cout << BLUE << "--- Confidence check started --- " << BLACK << std::endl;
		double confidence_interval = std::abs(1 / (N_subset * 1 / freq) - 1 / (N * 1 / freq));
		std::cout << BLUE << "  | frequency uncertatinty: " << confidence_interval << BLACK << std::endl;

		double A[N_subset], phi[N_subset], tot_A;
		std::complex<double> X[N_subset];
		std::cout << BLUE << "  | divided time windows: " << N_check << " each " << N_subset * 1 / freq << " sec long (Total = " << N_subset * N_check * 1 / freq << "sec, original = " << N * 1 / freq << "sec)" << BLACK << std::endl;
		std::cout << BLUE << "  | samples per time windows: " << N_subset << " from a total of " << N << " samples" << BLACK << std::endl;

		int thread = 0;
#ifdef OPENMP
#pragma omp parallel for
#endif
		for (int i = 0; i < N_check; i++)
		{
			DFT_Core(N_subset, b_sampled + i * N_subset, X, A, phi, tot_A);
			max = 0;
			index = 0;
			for (int p = 0; p < ceil(N_subset / 2); p++) // symetric-> only check half
			{
				if (max <= A[p] && p != 0)
				{
					max = A[p];
					index = p;
				}
			}
			check_freq = index / (N_subset * 1 / freq);

#ifdef OPENMP
			thread = omp_get_thread_num();
#endif
			if (dominant_freq - confidence_interval <= check_freq && check_freq <= dominant_freq + confidence_interval) // due to N_subset, the freq resolution changed
			{
				confidence += 100 / N_check;
				std::cout << BLUE << "  | " << i << ". thread " << thread << " dominant freq bin is " << index << " corresponding to frequency " << check_freq << " Hz -> " << GREEN << "\u2713" << BLACK << std::endl;
			}
			else
				std::cout << BLUE << "  | " << i << ". thread " << thread << " dominant freq bin is " << index << " corresponding to frequency " << check_freq << " Hz -> " << RED << "X" << BLACK << std::endl;
			fflush(stdout);
		}

		//? find samples: sample b with sample rate 1/freq to get b_sampled
		std::cout << BLUE << "--- Confidence check ended --- " << BLACK << std::endl;
		return confidence;
	}

	//**********************************************************************
	//*                       5. DFT_Create_String
	//**********************************************************************
	/**
	 * @brief Creates string for printing dft results
	 *
	 * @return string
	 */
	std::string DFT_Create_String(std::string name, double *p, int len, char *buf, int size, bool end)
	{
		std::string sp = "";

		if (len > 0)
			sp.append("\t\"" + name + "\" : [");
		// snprintf(buf, size, "\"%s\": [ ", name.c_str());

		for (int i = 0; i < len; i++)
		{
			if (i < len - 1)
				snprintf(buf, size, "%e, ", p[i]);
			else
				snprintf(buf, size, "%e", p[i]);
			sp.append(buf);
		}

		if (len > 0)
		{
			if (end)
				sp.append("]\n");
			else
				sp.append("],\n");
		}

		return sp;
	}
#endif

}