#include "qed_lib.h"

#define COUT_LOCK QED::ostream_lock(coutLock)
#define COUT_UNLOCK QED::ostream_unlock(coutLock)

pthread_mutex_t coutLock;
pthread_mutex_t drutLock;	//locker drutow ;)

QED_PREPARE_CTRL_C_HANDLER(bShouldRun);

const int ILE_KRAWCOWYCH = 5;
const int ILE_DRUTOW = 5;

int ileDrutowWolnych = ILE_DRUTOW;

int ileUszyly[ILE_KRAWCOWYCH];

void request_druty()
{
	bool bRunLoop = true;
	while(bRunLoop)
	{
		pthread_mutex_lock(&drutLock);
		assert( (ileDrutowWolnych >= 0) && "Katastrofa - ujemna liczba drutow ;)");
		if(ileDrutowWolnych >= 2)
		{
			ileDrutowWolnych -= 2;
			bRunLoop = false;
		}
		pthread_mutex_unlock(&drutLock);
	}
}

void release_druty()
{
	pthread_mutex_lock(&drutLock);

	ileDrutowWolnych += 2;
	assert((ileDrutowWolnych <= ILE_DRUTOW) && "Druty sie rozmnozyly...");

	pthread_mutex_unlock(&drutLock);
}

void* krawcowa(void* data)
{
	int numer = reinterpret_cast<int>(data);

	std::cout << COUT_LOCK << "Krawcowa " << numer << ": Gotowa do walki!" << std::endl << COUT_UNLOCK;

	while(true)
	{
		pthread_testcancel();	//point of cancelation ;)

		std::cout << COUT_LOCK << "Krawcowa " << numer << ": Szyje spodnie..." << std::endl << COUT_UNLOCK;
		sleep(std::rand() % 6 + 2);
		std::cout << COUT_LOCK << "Krawcowa " << numer << ": Uszylam spodnie! Czekam na druty." << std::endl << COUT_UNLOCK;

		//lock access to druty
		request_druty();
		std::cout << COUT_LOCK << "Krawcowa " << numer << ": Mam druty, doszywam guzik..." << std::endl << COUT_UNLOCK;
		sleep(std::rand() % 3 + 1);
		release_druty();

		std::cout << COUT_LOCK << "Krawcowa " << numer << ": Guzik przyszyty, druty zwolnione!" << std::endl << COUT_UNLOCK;

		++ileUszyly[numer];
	}
	
	return NULL;
}

int main()
{
	QED_INSTALL_CTRL_C_HANDLER;

	std::srand(std::time(NULL));

	memset(&ileUszyly, 0, ILE_KRAWCOWYCH*sizeof(int));

	pthread_t krawcowe[ILE_KRAWCOWYCH];
	pthread_mutex_init(&coutLock, NULL);
	pthread_mutex_init(&drutLock, NULL);

	std::cout << "//CTRL+C wychodzi i generuje podsumowanie prac." << std::endl;
	std::cout << "//W pechowych przypadkach CTRL+C moze zawiesic program na ktoryms join()'ie" << std::endl;
	std::cout << "//ale zwykle dziala i pokazuje poprawne statystyki ;)." << std::endl;
	sleep(2);


	QED_DOTIMES(ILE_KRAWCOWYCH, pthread_create(&krawcowe[i], NULL, krawcowa, reinterpret_cast<void*>(i)));

	QED_ACTIVE_SLEEP_WHILE(bShouldRun);

	std::cout << COUT_LOCK << "Sprzatanie zakladu... " << std::endl << COUT_UNLOCK;

	QED_DOTIMES(ILE_KRAWCOWYCH, pthread_cancel(krawcowe[i]));
	std::cout << COUT_LOCK << "Krawcowe wygonione..." << std::endl << COUT_UNLOCK;

	QED_DOTIMES(ILE_KRAWCOWYCH, pthread_join(krawcowe[i], NULL));
	std::cout << COUT_LOCK << "Zaklad zamkniety." << std::endl << COUT_UNLOCK;

	QED_DOTIMES(ILE_KRAWCOWYCH, std::cout << "Krawcowa " << i << " przyszyla " << ileUszyly[i] << " guzik(i/ow)." << std::endl);
	std::cout << "Jak widac - dziala ;) " << std::endl;

	return 0;
}
