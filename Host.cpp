#pragma warning (disable : 4996)//#define CL_USE_DEPRECATED_OPENCL_1_2_APIS//#define _CRT_SECURE_NO_WARNINGS//#define CL_QUEUE_PROFILING_ENABLE
#include <iostream>
#include <string.h>
#include <CL/cl.hpp>

struct Pixel
{
	unsigned char r, g, b;
};

char* citajKernelSource(const char* filename) //za citanje .cl programa
{
	FILE* fp;
	char* kernelSource = nullptr;
	long lenght = 0;
	if (fp = fopen(filename, "r"))
	{
		fseek(fp, 0, SEEK_END); //postavlja file pointer na kraj fajla (0 vrijednost znaci 0 od kraja fajla)
		lenght = ftell(fp); //govori trenutnu poziciju od pocekta
		rewind(fp);
		kernelSource = (char*)calloc(lenght, sizeof(char));
		if (kernelSource)
		{
			fread(kernelSource, 1, lenght, fp);
		}
		fclose(fp);
		return kernelSource;
	}
	std::cout << "Fajl nije pravilno otvoren!" << std::endl;
	return 0;
}
void napraviPrvuSlikuRucno(const char* filename, const int& visina,const int& sirina, unsigned char*& slika)
{
	std::string opcija;
	std::cout << "Unesite koordinate zivih celija: " << std::endl;
	opcija = "a";
	while (opcija != "n")
	{
		fflush(stdin); //cisti bafer za ulaz da budemo sigurni da se nece desiti greska pri unosu tako sto je slucajno ostala neka stara vrijednost sacuvanab od ranijeg unosa
	int x=-1;
	int y=-1;
		while ((x < 0 || x > (sirina - 1)) || (y < 0 || y > (visina - 1)))
		{
			std::cout << "x= ";
			fflush(stdin);
			std::cin >> x;
			std::cout<< std::endl;
			std::cout << "y= ";
			fflush(stdin);
			std::cin >> y;
			std::cout << std::endl;
		}
		slika[(y * sirina) + x] = 255;
		std::cout << "Novi unos? (y/n) -> "; 
		std::cin >> opcija;
		std::cout << std::endl;
	}	
}

void ucitajSliku(const char* filename, int& visina, int& sirina, unsigned char*& buffer)
{
	FILE* fp = fopen(filename, "rb");//binarno rb
	if (!fscanf(fp, "P5\n%d %d\n255\n", &sirina, &visina))
	{
		throw "error";
	}
	unsigned char* slika = new unsigned char[(size_t)visina * sirina];
	fread(slika, sizeof(unsigned char), (size_t)sirina * (size_t)visina, fp);//cita iz slike 
	fclose(fp);
	buffer = slika;
}
void ispisiSliku(const char* filename, const int visina, const int sirina, const unsigned char* buffer)
{
	FILE* fp = fopen(filename, "wb");
	fprintf(fp, "P5\n%d %d\n255\n", sirina, visina);
	fwrite(buffer, sizeof(unsigned char), (size_t)sirina * (size_t)visina, fp);
	fclose(fp);
}
void prelazakNaProizvoljuIteraciju(std::string imeFajla)
{
	int visina = -1;
	int sirina = -1;
	int iteracija = 0;
	unsigned char* slika = nullptr;
	std::cout << "Unesite iteraciju koju treba izracunati: " << std::endl;
	std::cin >> iteracija;
	ucitajSliku(imeFajla.c_str(), visina, sirina, slika);
	std::cout << "visina: " << visina << std::endl; std::cout << "sirina: " << sirina << std::endl;
	size_t velicinaSlike = visina * sirina;
	size_t globalVelicina[2], lokalVelicina[2]; //inicijalizovanje vrijednosti za spesifikaciju velicine radnih grupa i ukupan broj niti, nulti element predstavlja vrijednosti po x osi, a prvi po y
	cl_mem staraSlika;
	cl_mem novaSlika;
	cl_platform_id cpPlatform;
	cl_device_id device_id; //ovdje ce konkretno biti sacuvan id grafickog uredjaja
	cl_context context; //obezbjedjuje da izvrsavamo posao, kontekst je vezan za uredjaj
	cl_command_queue queue; //queue komandi
	cl_program program;
	cl_kernel kernel;
	cl_int err; //flag za greske
	cl_uint numOfPlatforms;
	lokalVelicina[0] = lokalVelicina[1] = 16;
	globalVelicina[0] = (size_t)ceil(sirina / (float)lokalVelicina[0]) * lokalVelicina[0];
	globalVelicina[1] = (size_t)ceil(visina / (float)lokalVelicina[1]) * lokalVelicina[1];
	char* kernelSource = citajKernelSource("CLProgram.cl");

	err = clGetPlatformIDs(1, &cpPlatform, NULL); // Bind to platform(dobijam nenula vrijednost ako se desi greska)(platforma je racunar)
	err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL); // Get ID for the device(uredjaj je gpu/cpu)
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err); // Create a context(vezuje platformu sa uredjajem)
	queue = clCreateCommandQueue(context, device_id, 0, &err); // Create a command queue/red sluzi za zakazivanje zadataka na uredjaj
	program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, &err); // Create the compute program from the source buffer
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL); // Build the program executable
	if (err) // Build log
	{
		size_t log_size; // Determine the size of the log
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char* log = (char*)malloc(log_size); // Allocate memory for the log
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL); // Get the log
		printf("%s\n", log);
		free(log);
	}
	kernel = clCreateKernel(program, "napraviNovuSliku", &err); // Create the compute kernel in the program
	//buffer memorija na grafickom uredjaju
	staraSlika = clCreateBuffer(context, CL_MEM_READ_WRITE, velicinaSlike, NULL, NULL); // alocira memoriju
	novaSlika = clCreateBuffer(context, CL_MEM_READ_WRITE, velicinaSlike, NULL, NULL);

	err |= clEnqueueWriteBuffer(queue, staraSlika, CL_TRUE, 0, velicinaSlike, slika, 0, NULL, NULL); // host(cpu) slaje podake na uredjajWrite data into the array in device memory

	for (int i = 0; i < iteracija; i++)
	{
		err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &staraSlika); // Set the arguments to the compute kernel
		err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &novaSlika);
		err |= clSetKernelArg(kernel, 2, sizeof(int), &visina);
		err |= clSetKernelArg(kernel, 3, sizeof(int), &sirina);
		// |= sprejacava da se izgubi vrijendost ako se desila greska 
		clFinish(queue);
		//zakazujem zadatke pomocu clEnqueueNDRangeKernel
		err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalVelicina, lokalVelicina, 0, NULL, NULL); // Execute the kernel over the entire range of the data set
		clFinish(queue); // Wait for the command queue to get serviced before reading back results

		clEnqueueReadBuffer(queue, novaSlika, CL_TRUE, 0, velicinaSlike, slika, 0, NULL, NULL); // Read data from the device memory into host memory
		clFinish(queue);

		cl_mem pom = staraSlika;
		staraSlika = novaSlika;
		novaSlika = pom;
	}
		std::string imeSlike = "slike\\slikaNakon" + std::to_string(iteracija) + "iteracija.pgm";
		ispisiSliku(imeSlike.c_str(), visina, sirina, slika);
	clReleaseMemObject(staraSlika); // Release OpenCL resources
	clReleaseMemObject(novaSlika);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);

	free(slika);

	return;
}

void ispisiPPMSliku(const char* filename, const struct Pixel* slika, const int sirina, const int visina)
{
	FILE* fp = fopen(filename, "wb");
	fprintf(fp, "P6\n%d %d\n255\n", sirina, visina);
	fwrite(slika, sizeof(Pixel), (size_t)sirina * (size_t)visina, fp);
	fclose(fp);
}

void detektovanjeOscilatora(std::string imeSlike)
{
	int visina = -1;
	int sirina = -1;
	unsigned char* slika = nullptr;
	struct Pixel* obojenaSlikaHost = nullptr;

	ucitajSliku(imeSlike.c_str(), visina, sirina, slika);
	std::cout << "Sirina: " << sirina << std::endl;
	std::cout << "Visina: " << visina << std::endl;

	size_t velicinaSlike = visina * sirina;

	obojenaSlikaHost = (Pixel*)calloc(velicinaSlike, sizeof(Pixel));

	size_t globalVelicina[2], lokalVelicina[2]; //inicijalizovanje vrijednosti za spesifikaciju velicine radnih grupa i ukupan broj niti, nulti element predstavlja vrijednosti po x osi, a prvi po y
	cl_mem staraSlika;
	cl_mem obojenaSlika;
	cl_platform_id cpPlatform;
	cl_device_id device_id; //ovdje ce konkretno biti sacuvan id grafickog uredjaja
	cl_context context; //obezbjedjuje da izvrsavamo posao, kontekst je vezan za uredjaj
	cl_command_queue queue; //queue komandi
	cl_program program;
	cl_kernel kernel;
	cl_int err; //flag za greske
	cl_uint numOfPlatforms;
	lokalVelicina[0] = lokalVelicina[1] = 8;
	globalVelicina[0] = (size_t)ceil(sirina / (float)lokalVelicina[0]) * lokalVelicina[0];
	globalVelicina[1] = (size_t)ceil(visina / (float)lokalVelicina[1]) * lokalVelicina[1];
	char* kernelSource = citajKernelSource("CLProgram.cl");

	err = clGetPlatformIDs(1, &cpPlatform, NULL); // Bind to platform(dobijam nenula vrijednost ako se desi greska)(platforma je racunar)
	err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL); // Get ID for the device(uredjaj je gpu/cpu)
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err); // Create a context(vezuje platformu sa uredjajem)
	queue = clCreateCommandQueue(context, device_id, 0, &err); // Create a command queue/red sluzi za zakazivanje zadataka na uredjaj
	program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, &err); // Create the compute program from the source buffer
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL); // Build the program executable
	if (err) // Build log
	{
		size_t log_size; // Determine the size of the log
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char* log = (char*)malloc(log_size); // Allocate memory for the log
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL); // Get the log
		printf("%s\n", log);
		free(log);
	}
	kernel = clCreateKernel(program, "oscilatorskiObrazac", &err); // Create the compute kernel in the program
	//buffer memorija na grafickom uredjaju
	staraSlika = clCreateBuffer(context, CL_MEM_READ_ONLY, velicinaSlike, NULL, NULL); // alocira memoriju
	obojenaSlika = clCreateBuffer(context, CL_MEM_WRITE_ONLY, velicinaSlike * sizeof(Pixel), NULL, NULL);

	err |= clEnqueueWriteBuffer(queue, staraSlika, CL_TRUE, 0, velicinaSlike, slika, 0, NULL, NULL); // host(cpu) slaje podake na uredjajWrite data into the array in device memory
	clFinish(queue);

	err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &staraSlika); // Set the arguments to the compute kernel
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &obojenaSlika);
	err |= clSetKernelArg(kernel, 2, sizeof(int), &sirina);
	err |= clSetKernelArg(kernel, 3, sizeof(int), &visina);

	// |= sprejacava da se izgubi vrijendost ako se desila greska 
	clFinish(queue);
	//zakazujem zadatke pomocu clEnqueueNDRangeKernel
	err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalVelicina, lokalVelicina, 0, NULL, NULL); // Execute the kernel over the entire range of the data set
	clFinish(queue); // Wait for the command queue to get serviced before reading back results

	clEnqueueReadBuffer(queue, obojenaSlika, CL_TRUE, 0, velicinaSlike * sizeof(Pixel), obojenaSlikaHost, 0, NULL, NULL); // Read data from the device memory into host memory
	clFinish(queue);

	imeSlike.replace(imeSlike.end() - 4, imeSlike.end(), ".ppm"); // mijenja ekstenziju iz .pgm u .ppm
	imeSlike.replace(imeSlike.begin(), imeSlike.begin() + 6, ""); //brise ime foldera u nazivu slike

	std::string imeSlikePod = "slike\\Detektovani_Obrasci_" + imeSlike;
	ispisiPPMSliku(imeSlikePod.c_str(), obojenaSlikaHost, sirina, visina);

	clReleaseMemObject(staraSlika); // Release OpenCL resources
	clReleaseMemObject(obojenaSlika);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);

	free(slika);
	free(obojenaSlikaHost);
}



void detektovanjeOscilatoraSledeceIteracije(std::string imeSlike)
{
	int visina = -1;
	int sirina = -1;
	unsigned char* slika = nullptr;
	struct Pixel* obojenaSlikaHost = nullptr;

	ucitajSliku(imeSlike.c_str(), visina, sirina, slika);
	std::cout << "Sirina: " << sirina << std::endl;
	std::cout << "Visina: " << visina << std::endl;

	size_t velicinaSlike = visina * sirina;

	obojenaSlikaHost = (Pixel*)calloc(velicinaSlike, sizeof(Pixel));

	size_t globalVelicina[2], lokalVelicina[2]; //inicijalizovanje vrijednosti za spesifikaciju velicine radnih grupa i ukupan broj niti, nulti element predstavlja vrijednosti po x osi, a prvi po y
	cl_mem staraSlika;
	cl_mem novaSlika;
	cl_mem obojenaSlika;
	cl_platform_id cpPlatform;
	cl_device_id device_id; //ovdje ce konkretno biti sacuvan id grafickog uredjaja
	cl_context context; //obezbjedjuje da izvrsavamo posao, kontekst je vezan za uredjaj
	cl_command_queue queue; //queue komandi
	cl_program program;
	cl_kernel kernel;
	cl_int err; //flag za greske
	cl_uint numOfPlatforms;
	lokalVelicina[0] = lokalVelicina[1] = 8;
	globalVelicina[0] = (size_t)ceil(sirina / (float)lokalVelicina[0]) * lokalVelicina[0];
	globalVelicina[1] = (size_t)ceil(visina / (float)lokalVelicina[1]) * lokalVelicina[1];
	char* kernelSource = citajKernelSource("CLProgram.cl");

	err = clGetPlatformIDs(1, &cpPlatform, NULL); // Bind to platform(dobijam nenula vrijednost ako se desi greska)(platforma je racunar)
	err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL); // Get ID for the device(uredjaj je gpu/cpu)
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err); // Create a context(vezuje platformu sa uredjajem)
	queue = clCreateCommandQueue(context, device_id, 0, &err); // Create a command queue/red sluzi za zakazivanje zadataka na uredjaj
	program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, &err); // Create the compute program from the source buffer
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL); // Build the program executable
	if (err) // Build log
	{
		size_t log_size; // Determine the size of the log
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char* log = (char*)malloc(log_size); // Allocate memory for the log
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL); // Get the log
		printf("%s\n", log);
		free(log);
	}
	kernel = clCreateKernel(program, "sledecaIteracijaUzDetekciju", &err); // Create the compute kernel in the program
	//buffer memorija na grafickom uredjaju
	staraSlika = clCreateBuffer(context, CL_MEM_READ_ONLY, velicinaSlike, NULL, NULL); // alocira memoriju
	novaSlika = clCreateBuffer(context, CL_MEM_READ_WRITE, velicinaSlike, NULL, NULL);
	obojenaSlika = clCreateBuffer(context, CL_MEM_WRITE_ONLY, velicinaSlike * sizeof(Pixel), NULL, NULL);

	err |= clEnqueueWriteBuffer(queue, staraSlika, CL_TRUE, 0, velicinaSlike, slika, 0, NULL, NULL); // host(cpu) slaje podake na uredjajWrite data into the array in device memory
	clFinish(queue);

	err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &staraSlika);// Set the arguments to the compute kernel
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &novaSlika);
	err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &obojenaSlika);
	err |= clSetKernelArg(kernel, 3, sizeof(int), &visina);
	err |= clSetKernelArg(kernel, 4, sizeof(int), &sirina);

	// |= sprejacava da se izgubi vrijendost ako se desila greska 
	clFinish(queue);
	//zakazujem zadatke pomocu clEnqueueNDRangeKernel
	err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalVelicina, lokalVelicina, 0, NULL, NULL); // Execute the kernel over the entire range of the data set
	clFinish(queue); // Wait for the command queue to get serviced before reading back results

	clEnqueueReadBuffer(queue, obojenaSlika, CL_TRUE, 0, velicinaSlike * sizeof(Pixel), obojenaSlikaHost, 0, NULL, NULL); // Read data from the device memory into host memory
	clFinish(queue);

	imeSlike.replace(imeSlike.end() - 4, imeSlike.end(), ".ppm"); // mijenja ekstenziju iz .pgm u .ppm
	imeSlike.replace(imeSlike.begin(), imeSlike.begin() + 6, ""); //brise ime foldera u nazivu slike

	std::string imeSlikePod = "slike\\Detektovani_Obrasci_Naredne_iteracije" + imeSlike;
	ispisiPPMSliku(imeSlikePod.c_str(), obojenaSlikaHost, sirina, visina);

	clReleaseMemObject(staraSlika); // Release OpenCL resources
	clReleaseMemObject(novaSlika);
	clReleaseMemObject(obojenaSlika);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);

	free(slika);
	free(obojenaSlikaHost);
}
void ispisSvakeIteracije(std::string imePrveSlike)
{
	int visina = -1;
	int sirina = -1;
	int brojIteracija = 0;
	std::string opcija;
	std::cout << "Unesite broj iteracija: " << std::endl;
	std::cin >> brojIteracija;
	unsigned char* slika = nullptr;
	std::cout << "Da li zelite napraviti prvu sliku rucno? (y/n)->" << std::endl;
	std::cin >> opcija;
	if(opcija=="y")
	{
		std::cout << "Unesite dimenzije: " << std::endl;
		std::cout << "visina= ";
		std::cin >> visina;
		std::cout << std::endl;
		std::cout << "sirina= ";
		std::cin >> sirina;
		slika = (unsigned char*)calloc((sirina * visina), sizeof(unsigned char));
		napraviPrvuSlikuRucno(imePrveSlike.c_str(), visina, sirina, slika);
		ispisiSliku(imePrveSlike.c_str(), visina, sirina, slika);
	}else 
		ucitajSliku(imePrveSlike.c_str(), visina, sirina, slika);
	std::cout << "visina: " << visina << std::endl; std::cout << "sirina: " << sirina << std::endl;
	size_t velicinaSlike = visina * sirina;
	size_t globalVelicina[2], lokalVelicina[2]; //inicijalizovanje vrijednosti za spesifikaciju velicine radnih grupa i ukupan broj niti, nulti element predstavlja vrijednosti po x osi, a prvi po y
	cl_mem staraSlika; //pokazivac na memoriju u grafickoj kartici (graficki bafer)
	cl_mem novaSlika;
	cl_platform_id cpPlatform;
	cl_device_id device_id; //ovdje ce konkretno biti sacuvan id grafickog uredjaja
	cl_context context; //obezbjedjuje da izvrsavamo posao, kontekst je vezan za uredjaj
	cl_command_queue queue; //queue komandi
	cl_program program;
	cl_kernel kernel;
	cl_int err; //flag za greske
	cl_uint numOfPlatforms;
	lokalVelicina[0] = lokalVelicina[1] = 16;
	globalVelicina[0] = (size_t)ceil(sirina / (float)lokalVelicina[0]) * lokalVelicina[0];
	globalVelicina[1] = (size_t)ceil(visina / (float)lokalVelicina[1]) * lokalVelicina[1];
	char* kernelSource = citajKernelSource("CLProgram.cl");

	err = clGetPlatformIDs(1, &cpPlatform, NULL); // Bind to platform(dobijam nenula vrijednost ako se desi greska)(platforma je racunar)
	err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL); // Get ID for the device(uredjaj je gpu/cpu)
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err); // Create a context(vezuje platformu sa uredjajem)
	queue = clCreateCommandQueue(context, device_id, 0, &err); // Create a command queue/red sluzi za zakazivanje zadataka na uredjaj
	program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, &err); // Create the compute program from the source buffer
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL); // Build the program executable
	if (err) // Build log
	{
		size_t log_size; // Determine the size of the log
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char* log = (char*)malloc(log_size); // Allocate memory for the log
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL); // Get the log
		printf("%s\n", log);
		free(log);
	}
	kernel = clCreateKernel(program, "napraviNovuSliku", &err); // Create the compute kernel in the program
	//buffer memorija na grafickom uredjaju
	staraSlika = clCreateBuffer(context, CL_MEM_READ_WRITE, velicinaSlike, NULL, NULL); // alocira memoriju na grafickom uredjaju
	novaSlika = clCreateBuffer(context, CL_MEM_READ_WRITE, velicinaSlike, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, staraSlika, CL_TRUE, 0, velicinaSlike, slika, 0, NULL, NULL); // host(cpu) slaje podake na uredjaj

	for (int i = 0; i < brojIteracija; i++)
	{
		err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &staraSlika); // Set the arguments to the compute kernel
		err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &novaSlika);
		err |= clSetKernelArg(kernel, 2, sizeof(int), &visina);
		err |= clSetKernelArg(kernel, 3, sizeof(int), &sirina);
		// |= sprejacava da se izgubi vrijendost ako se desila greska 
		clFinish(queue);
		//zakazujem zadatke pomocu clEnqueueNDRangeKernel
		err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalVelicina, lokalVelicina, 0, NULL, NULL); // Pokrece izvrsavanje funkcije nad svim proslijedjenim podacima
		clFinish(queue); // Wait for the command queue to get serviced before reading back results

		clEnqueueReadBuffer(queue, novaSlika, CL_TRUE, 0, velicinaSlike, slika, 0, NULL, NULL); // Read data from the device memory into host memory
		clFinish(queue);
		std::string imeSlike = "slike\\slika" + std::to_string(i + 1) + ".pgm";
		ispisiSliku(imeSlike.c_str(), visina, sirina, slika);
		//detektovanjeOscilatora(imeSlike);
		
		imePrveSlike = imeSlike;
		cl_mem pom = staraSlika;
		staraSlika = novaSlika;
		novaSlika = pom;
	}

	clReleaseMemObject(staraSlika); // Release OpenCL resources
	clReleaseMemObject(novaSlika);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);

	free(slika);

	
}

void podsegment(std::string imeSlike, std::string imeCrne)
{
	int visina = -1;
	int sirina = -1;
	int x1 = -1;
	int y1 = -1;
	int x2 = -1;
	int y2 = -1;
	unsigned char* slika = nullptr;
	unsigned char* slikacrna = nullptr;
	unsigned char* podsegmentSlikaPom = nullptr;
	
	ucitajSliku(imeSlike.c_str(), visina, sirina, slika);
	std::cout << "sirina: " << sirina << std::endl;
	std::cout << "visina: " << visina << std::endl;
	ucitajSliku(imeCrne.c_str(), visina, sirina, slikacrna);
	std::cout << "sirina: " << sirina << std::endl;
	std::cout << "visina: " << visina << std::endl;

	std::cout << "Unesite koordinate podsegmenta" << std::endl;
	do
	{
		std::cout << "x1="; std::cin >> x1; std::cout << std::endl;

		std::cout << "y1="; std::cin >> y1; std::cout << std::endl;

		std::cout << "x2="; std::cin >> x2; std::cout << std::endl;

		std::cout << "y2="; std::cin >> y2; std::cout << std::endl;
	} while (x1<0 || y1<0 || x2 < 0 || y2 < 0 || x1>(sirina) || y1>(visina) || x2>(sirina) || y2>(visina) || x2<=x1 || y2<=y1);
	size_t velicinaSlike = visina * sirina;
	size_t sirinaPodsegmenta = x2 - x1 + 1;
	size_t visinaPodsegmenta = y2 - y1 + 1;
	size_t velicinaPodsegmenta = sirinaPodsegmenta * visinaPodsegmenta;
	podsegmentSlikaPom = (unsigned char*)calloc(velicinaPodsegmenta, sizeof(unsigned char));

	size_t globalVelicina[2], lokalVelicina[2]; //inicijalizovanje vrijednosti za spesifikaciju velicine radnih grupa i ukupan broj niti, nulti element predstavlja vrijednosti po x osi, a prvi po y
	cl_mem staraSlika;
	cl_mem podsegmentSlika;
	cl_platform_id cpPlatform;
	cl_device_id device_id; //ovdje ce konkretno biti sacuvan id grafickog uredjaja
	cl_context context; //obezbjedjuje da izvrsavamo posao, kontekst je vezan za uredjaj
	cl_command_queue queue; //queue komandi
	cl_program program;
	cl_kernel kernel;
	cl_int err; //flag za greske
	cl_uint numOfPlatforms;
	lokalVelicina[0] = lokalVelicina[1] = 16;
	globalVelicina[0] = (size_t)ceil(sirina / (float)lokalVelicina[0]) * lokalVelicina[0];
	globalVelicina[1] = (size_t)ceil(visina / (float)lokalVelicina[1]) * lokalVelicina[1];
	char* kernelSource = citajKernelSource("CLProgram.cl");

	err = clGetPlatformIDs(1, &cpPlatform, NULL); // Bind to platform(dobijam nenula vrijednost ako se desi greska)(platforma je racunar)
	err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL); // Get ID for the device(uredjaj je gpu/cpu)
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err); // Create a context(vezuje platformu sa uredjajem)
	queue = clCreateCommandQueue(context, device_id, 0, &err); // Create a command queue/red sluzi za zakazivanje zadataka na uredjaj
	program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, &err); // Create the compute program from the source buffer
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL); // Build the program executable
	if (err) // Build log
	{
		size_t log_size; // Determine the size of the log
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char* log = (char*)malloc(log_size); // Allocate memory for the log
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL); // Get the log
		printf("%s\n", log);
		free(log);
	}
	kernel = clCreateKernel(program, "dohvatiPodsegment", &err); // Create the compute kernel in the program
	//buffer memorija na grafickom uredjaju
	staraSlika = clCreateBuffer(context, CL_MEM_READ_ONLY, velicinaSlike, NULL, NULL); // alocira memoriju
	podsegmentSlika = clCreateBuffer(context, CL_MEM_WRITE_ONLY, velicinaPodsegmenta, NULL, NULL);

	err |= clEnqueueWriteBuffer(queue, staraSlika, CL_TRUE, 0, velicinaSlike, slika, 0, NULL, NULL); // host(cpu) slaje podake na uredjajWrite data into the array in device memory
	err |= clEnqueueWriteBuffer(queue, podsegmentSlika, CL_TRUE, 0, velicinaSlike, slikacrna, 0, NULL, NULL); // host(cpu) slaje podake na uredjajWrite data into the array in device memory
	clFinish(queue);
	
		err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &staraSlika); // Set the arguments to the compute kernel
		err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &podsegmentSlika);
		err |= clSetKernelArg(kernel, 2, sizeof(int), &visina);
		err |= clSetKernelArg(kernel, 3, sizeof(int), &sirina);

		err |= clSetKernelArg(kernel, 4, sizeof(int), &x1);
		err |= clSetKernelArg(kernel, 5, sizeof(int), &y1);
		err |= clSetKernelArg(kernel, 6, sizeof(int), &x2);
		err |= clSetKernelArg(kernel, 7, sizeof(int), &y2);
		// |= sprejacava da se izgubi vrijendost ako se desila greska 
		clFinish(queue);
		//zakazujem zadatke pomocu clEnqueueNDRangeKernel
		err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalVelicina, lokalVelicina, 0, NULL, NULL); // Execute the kernel over the entire range of the data set
		clFinish(queue); // Wait for the command queue to get serviced before reading back results
		//slika = (unsigned char*)calloc(velicinaPodsegmenta, sizeof(unsigned char));
		clEnqueueReadBuffer(queue, podsegmentSlika, CL_TRUE, 0, velicinaPodsegmenta, podsegmentSlikaPom, 0, NULL, NULL); // Read data from the device memory into host memory
		clFinish(queue);

		imeSlike.replace(imeSlike.begin(), imeSlike.begin() + 6, "");
		std::string imeSlikePod = "slike\\podsegment_" + imeSlike;
		ispisiSliku(imeSlikePod.c_str(), visinaPodsegmenta-1, sirinaPodsegmenta-1, podsegmentSlikaPom);
	
			
	clReleaseMemObject(staraSlika); // Release OpenCL resources
	clReleaseMemObject(podsegmentSlika);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);

	free(slika);
	free(podsegmentSlikaPom);
}


int main()
{
	//ispisSvakeIteracije("slike\\slika0.pgm");//ovde moze da pravi sliku rucno
	//prelazakNaProizvoljuIteraciju("slike\\slika0.pgm");
	//podsegment("slike\\slika15.pgm","slike\\slika1.pgm");
	//podsegment("slike<\\slika0.pgm");
	//detektovanjeOscilatora("slike\\slikaNakon1000iteracija.pgm");
	detektovanjeOscilatora("slike\\slika0.pgm");
	//detektovanjeOscilatora("slike\\slikaNakon11iteracija.pgm");
	//detektovanjeOscilatoraSledeceIteracije("slike\\slikaNakon10iteracija.pgm");
	
		return 0;
}

