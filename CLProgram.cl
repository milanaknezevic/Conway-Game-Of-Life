struct Pixel
{
	unsigned char r, g, b;
};







__kernel void napraviNovuSliku(__global unsigned char* staraSlika, __global unsigned char* novaSlika, const int visina, const int sirina)
{//ziva=255 mrtva=0
	int x = get_global_id(0);//x se mjenja po sirini a y po visini
	int y = get_global_id(1);

	if (x <= 0 || y <= 0 || x >= sirina - 1 || y >= visina - 1)
	{//novaSlika[(y * sirina) + x] = 0;//odbacivanje rubova = rubni pikseli su mrtvi
		return;
	}

	int brojZivihSusjeda = 0;
	if (staraSlika[((y - 1) * sirina) + (x - 1)] == 255) brojZivihSusjeda++;//lijevo gore
	if (staraSlika[(y * sirina) + (x - 1)] == 255) brojZivihSusjeda++;//lijevo
	if (staraSlika[((y + 1) * sirina) + (x - 1)] == 255) brojZivihSusjeda++;//lijevo dole

	if (staraSlika[((y - 1) * sirina) + x] == 255) brojZivihSusjeda++;//gore
	if (staraSlika[((y + 1) * sirina) + x] == 255) brojZivihSusjeda++;//dole

	if (staraSlika[((y - 1) * sirina) + (x + 1)] == 255) brojZivihSusjeda++;//desno gore
	if (staraSlika[(y * sirina) + (x + 1)] == 255) brojZivihSusjeda++;//desno
	if (staraSlika[((y + 1) * sirina) + (x + 1)] == 255) brojZivihSusjeda++;//desno gore

	if ((staraSlika[(y * sirina) + x] == 255) && ((brojZivihSusjeda < 2) || (brojZivihSusjeda > 3)))// Svaka ziva celija sa manje od dva ziva susjeda postaje neziva.
	{

		novaSlika[(y * sirina) + x] = 0;//
		
	}
	else if ((staraSlika[(y * sirina) + x] == 255) && (brojZivihSusjeda == 2 || brojZivihSusjeda == 3))// Svaka ziva celija sa dva ili vise zivih susjeda prenosi se u sljedecu iteraciju.
	{																	 //Svaka ziva celija sa vise od tri ziva susjeda postaje neziva.
		novaSlika[(y * sirina) + x] = 255;
	}
	else if ((staraSlika[(y * sirina) + x] == 0) && (brojZivihSusjeda == 3))// Svaka neziva celija sa tacno tri ziva susjeda postaje ziva
	{
		novaSlika[(y * sirina) + x] = 255;
	}
	else
		novaSlika[(y * sirina) + x] = 0;


	return;
}

__kernel void dohvatiPodsegment(__global unsigned char* staraSlika, __global unsigned char* novaSlika, const int staraVisina, const int staraSirina, const int x1, const int y1, const int x2, const int y2)
{
	int xp = get_global_id(0);//vrijednsoti podsegmenta tj trenutne vrijednosti u originalnoj slici 
	int yp = get_global_id(1);

	if (yp < y1 || yp > y2 || xp < x1 || xp > x2)
		return;

	int sirinaPodsegmenta = x2 - x1;
	int xt = xp - x1;//translirane koordinate za novu sliku koja je podsegment originalne
	int yt = yp - y1;

	novaSlika[yt * sirinaPodsegmenta + xt] = staraSlika[(yp * staraSirina) + xp];


}


bool isBlinkerHorizontal(__global unsigned char* staraSlika,const int x,const int y,const int sirina)
{
	bool isBlinker = false;
	if (staraSlika[(y * sirina) + x] == 255 
		&& staraSlika[(y * sirina) + (x - 1)] == 255 
		&& staraSlika[(y * sirina) + (x + 1)] == 255		//ziva 3

		&& staraSlika[((y - 1) * sirina) + (x - 2)] == 0
		&& staraSlika[((y - 1) * sirina) + (x - 1)] == 0
		&& staraSlika[((y - 1) * sirina) + x ] == 0			//gornji red mrtav
		&& staraSlika[((y - 1) * sirina) + (x +1)] == 0
		&& staraSlika[((y - 1) * sirina) + (x +2)] == 0

		&& staraSlika[(y * sirina) + (x +2)] == 0			//sa strana mrtvo
		&& staraSlika[(y * sirina) + (x - 2)] == 0

		&& staraSlika[((y +1) * sirina) + (x - 2)] == 0			
		&& staraSlika[((y +1) * sirina) + (x - 1)] == 0
		&& staraSlika[((y +1) * sirina) + x] == 0			//donji red mrtav
		&& staraSlika[((y + 1) * sirina) + (x+1)] == 0
		&& staraSlika[((y + 1) * sirina) + (x + 2)] == 0
		)
		isBlinker = true;

	return isBlinker;


}




bool isBlinkerVertical(__global unsigned char* staraSlika, const int x, const int y, const int sirina)
{
	bool isBlinker = false;
	if (staraSlika[(y * sirina) + x] == 255
		&& staraSlika[((y-1)* sirina) + x ] == 255
		&& staraSlika[((y + 1) * sirina) + x ] == 255		//ziva 3
		

		&& staraSlika[((y - 2) * sirina) + (x - 1)] == 0
		&& staraSlika[((y - 1) * sirina) + (x - 1)] == 0
		&& staraSlika[(y * sirina) + (x - 1)] == 0			//lijevo mrtvi
		&& staraSlika[((y + 1) * sirina) + (x - 1)] == 0
		&& staraSlika[((y + 2) * sirina) + (x - 1)] == 0

		&& staraSlika[((y-2) * sirina) + x ] == 0			//gornji i donji mrtvi
		&& staraSlika[((y+2) * sirina) + x ] == 0

		&& staraSlika[((y -2) * sirina) + (x + 1)] == 0
		&& staraSlika[((y -1) * sirina) + (x + 1)] == 0
		&& staraSlika[(y * sirina) + (x + 1)] == 0			//desno mrtvi
		&& staraSlika[((y + 1) * sirina) + (x + 1)] == 0
		&& staraSlika[((y + 2) * sirina) + (x + 1)] == 0
		)
		isBlinker = true;

	return isBlinker;


}


__kernel void oscilatorskiObrazac(__global unsigned char* staraSlika, __global struct Pixel* novaSlika, const int sirina, const int visina)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (staraSlika[(y * sirina) + x] == 255)
	{
		novaSlika[(y * sirina) + x].r = 255;
		novaSlika[(y * sirina) + x].g = 255;
		novaSlika[(y * sirina) + x].b = 255;
	}
	else
	{
		novaSlika[(y * sirina) + x].r = 0;
		novaSlika[(y * sirina) + x].g = 0;
		novaSlika[(y * sirina) + x].b = 0;
	}
	barrier(CLK_GLOBAL_MEM_FENCE);

	if (!((x < 1 || y < 2) || (x > (sirina - 2) || y > (visina - 3))) && isBlinkerVertical(staraSlika, x, y, sirina) == true)//ako je pronadjen odgovarajuci blinker u mogucim koordinatama
	{
		novaSlika[(y * sirina) + x].r = 255;
		novaSlika[(y * sirina) + x].g = 0;
		novaSlika[(y * sirina) + x].b = 0;

		novaSlika[((y-1) * sirina) + x].r = 255;
		novaSlika[((y-1) * sirina) + x].g = 0;
		novaSlika[((y-1) * sirina) + x].b = 0;

		novaSlika[((y + 1) * sirina) + x].r = 255;
		novaSlika[((y + 1) * sirina) + x].g = 0;
		novaSlika[((y + 1) * sirina) + x].b = 0;
	}
	else if (!((x < 2 || y < 1) || (x>(sirina-3) || y>(visina-2) )) &&  isBlinkerHorizontal(staraSlika,x,y,sirina)==true )
	{
		novaSlika[(y * sirina) + x].r = 255;
		novaSlika[(y * sirina) + x].g = 0;
		novaSlika[(y * sirina) + x].b = 0;

		novaSlika[(y * sirina) + (x-1)].r = 255;
		novaSlika[(y * sirina) + (x-1)].g = 0;
		novaSlika[(y * sirina) + (x-1)].b = 0;

		novaSlika[(y * sirina) + (x+1)].r = 255;
		novaSlika[(y * sirina) + (x+1)].g = 0;
		novaSlika[(y * sirina) + (x+1)].b = 0;
	}
	return;
}

__kernel void sledecaIteracijaUzDetekciju(__global unsigned char* staraSlika, __global unsigned char* novaSlika, __global struct Pixel* obojena, const int visina, const int sirina)
{
	int x = get_global_id(0);//x se mjenja po sirini a y po visini
	int y = get_global_id(1);

	if (x <= 0 || y <= 0 || x >= sirina - 1 || y >= visina - 1)
	{
		//novaSlika[(y * sirina) + x] = 0;//odbacivanje rubova = rubni pikseli su mrtvi
		return;
	}

	__private int brojZivihSusjeda = 0;
	if (staraSlika[((y - 1) * sirina) + (x - 1)] == 255) brojZivihSusjeda++;//lijevo gore
	if (staraSlika[(y * sirina) + (x - 1)] == 255) brojZivihSusjeda++;//lijevo
	if (staraSlika[((y + 1) * sirina) + (x - 1)] == 255) brojZivihSusjeda++;//lijevo dole

	if (staraSlika[((y - 1) * sirina) + x] == 255) brojZivihSusjeda++;//gore
	if (staraSlika[((y + 1) * sirina) + x] == 255) brojZivihSusjeda++;//dole

	if (staraSlika[((y - 1) * sirina) + (x + 1)] == 255) brojZivihSusjeda++;//desno gore
	if (staraSlika[(y * sirina) + (x + 1)] == 255) brojZivihSusjeda++;//desno
	if (staraSlika[((y + 1) * sirina) + (x + 1)] == 255) brojZivihSusjeda++;//desno gore
	barrier(CLK_GLOBAL_MEM_FENCE);

	if ((staraSlika[(y * sirina) + x] == 255) && ((brojZivihSusjeda < 2) || (brojZivihSusjeda > 3)))// Svaka ziva celija sa manje od dva ziva susjeda postaje neziva.
	{

		novaSlika[(y * sirina) + x] = 0;
	}
	else if ((staraSlika[(y * sirina) + x] == 255) && (brojZivihSusjeda == 2 || brojZivihSusjeda == 3))// Svaka ziva celija sa dva ili vise zivih susjeda prenosi se u sljedecu iteraciju.
	{																	 //Svaka ziva celija sa vise od tri ziva susjeda postaje neziva.
		novaSlika[(y * sirina) + x] = 255;
	}
	else if ((staraSlika[(y * sirina) + x] == 0) && (brojZivihSusjeda == 3))// Svaka neziva celija sa tacno tri ziva susjeda postaje ziva
	{
		novaSlika[(y * sirina) + x] = 255;
	}
	else {
		novaSlika[(y * sirina) + x] = 0;
	}
	
	barrier(CLK_GLOBAL_MEM_FENCE);




	if (novaSlika[(y * sirina) + x] == 255)
	{
		obojena[(y * sirina) + x].r = 255;
		obojena[(y * sirina) + x].g = 255;
		obojena[(y * sirina) + x].b = 255;
	}
	else
	{
		obojena[(y * sirina) + x].r = 0;
		obojena[(y * sirina) + x].g = 0;
		obojena[(y * sirina) + x].b = 0;
	}
	barrier(CLK_GLOBAL_MEM_FENCE);

	if (!((x < 1 || y < 2) || (x > (sirina - 2) || y > (visina - 3))) && isBlinkerVertical(novaSlika, x, y, sirina) == true)//ako je pronadjen odgovarajuci blinker u mogucim koordinatama
	{
		obojena[(y * sirina) + x].r = 255;
		obojena[(y * sirina) + x].g = 0;
		obojena[(y * sirina) + x].b = 0;

		obojena[((y - 1) * sirina) + x].r = 255;
		obojena[((y - 1) * sirina) + x].g = 0;
		obojena[((y - 1) * sirina) + x].b = 0;

		obojena[((y + 1) * sirina) + x].r = 255;
		obojena[((y + 1) * sirina) + x].g = 0;
		obojena[((y + 1) * sirina) + x].b = 0;
	}
	else if (!((x < 2 || y < 1) || (x > (sirina - 3) || y > (visina - 2))) && isBlinkerHorizontal(novaSlika, x, y, sirina) == true)
	{
		obojena[(y * sirina) + x].r = 255;
		obojena[(y * sirina) + x].g = 0;
		obojena[(y * sirina) + x].b = 0;

		obojena[(y * sirina) + (x - 1)].r = 255;
		obojena[(y * sirina) + (x - 1)].g = 0;
		obojena[(y * sirina) + (x - 1)].b = 0;

		obojena[(y * sirina) + (x + 1)].r = 255;
		obojena[(y * sirina) + (x + 1)].g = 0;
		obojena[(y * sirina) + (x + 1)].b = 0;
	}
	
	return;
}

