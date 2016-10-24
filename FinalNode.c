//File untuk membuat sebuah node/server yang dapat dimonitor munin-master/client
//dengan cara melakukan binding port 4949

//Author
//Geraldi Dzakwan 		13514065
//Ramos Janoah H 		13514089
//Alvin Junianto Lan 	13514105

//Macro untuk mendapatkan akses fungsi asprintf
#define _GNU_SOURCE
//Untuk standard input output
//Misalnya, variabel FILE dan fungsi printf, popen, pclose, fgets
#include <stdio.h>
//Untuk library standar
//Misalnya, variabel NULL dan fungsi atoi, exit
#include <stdlib.h>
//Untuk manipulasi string
//Semisal fungsi bzero, strcmp, strtok, strlen
#include <string.h>
//Untuk fungsi-fungsi standar
//Semisal write, read, close, gethostname
#include <unistd.h>
//Untuk keperluan socket programming
//Untuk variabel INADDR_ANY, sockaddr_in
//Untuk variabel SOCK_STREAM dan fungsi bind, accept, listen, socket, htons
#include <arpa/inet.h>
//Untuk keperluan threading
//Untuk variabel semisal pthread_t dan fungsi semisal pthread_create
#include <pthread.h>

//Variabel untuk menghitung jumlah koneksi yang terbentuk
int connectionCount = 0;

//Fungsi untuk print error dan kemudian keluar program
void printError(const char *message);

//Fungsi untuk menerima koneksi dari munin-master baru
//dan melakukan handling untuk input/perintah dari munin-master
//Fungsi dijalankan dengan thread untuk memungkinkan
//adanya beberapa koneksi dengan munin-master sekaligus
void* acceptMaster(void*);

void messageHandler(int, char*, char*, char*, int);

//Fungsi main
int main(int argc, char *argv[]) {
    //Variabel thread untuk menampung hingga 50 koneksi
	pthread_t connectionThread[50];

	//Variabel port yang akan di-bind : 4949
    //Variabel socket untuk node & master
	int portNumber = 4949, nodeSocketFD, masterSocketFD;
    //Array untuk mencatat seluruh master socket yang terkoneksi ke node
    int masterSocketArray[50];

	//Membuat struct local protocol address (IP & Port) untuk node & master
    struct sockaddr_in nodeAddress, masterAddress;

	//Membuat socket untuk node/server
    //AF_INET untuk internet-based applications -> IPv4 protocol
    //SOCK_STREAM, socket menggunakan TCP untuk data transmission
    //0, menetapkan default protocol untuk family AF_INET dan tipe SOCK_STREAM
	if ((nodeSocketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        //Jika gagal membuat socket, keluar program
		printError("Socket gagal dibuat! Keluar program, node berhenti.\n");
	}

	//Melakukan inisialisasi nodeAddress
    //Mengeset seluruh char menjadi empty char -> 0
	bzero((char*) &nodeAddress, sizeof(nodeAddress));

	//Mengeset family nodeAddress menjadi AF_INET
	nodeAddress.sin_family = AF_INET;
	//Mengeset address nodeAddress secara otomatis
	nodeAddress.sin_addr.s_addr = INADDR_ANY;
	//Mengeset port nodeAddress menjadi 4949
	nodeAddress.sin_port = htons(portNumber);

	//Melakukan binding, yakni meng-assign local protocol address ke socket
	if (bind(nodeSocketFD, (struct sockaddr*) &nodeAddress, sizeof(nodeAddress)) < 0) {	
        //Jika gagal binding socket, keluar program
        printError("Gagal melakukan binding node address ke socket! Keluar program, node berhenti.\n");
	}
	
	//Fungsi yang khusus dilakukan TCP Server
    //Berfungsi agar node socket siap menerima koneksi yang diarahkan kepadanya
    //Parameter 10 artinya maksimum 10 koneksi yang dapat diletakkan pada queue
    //(dengan kata lain, socket hanya bisa menerima maksimum 10 koneksi secara bersamaan)
    if (listen(nodeSocketFD, 10) < 0) {
        //Jika gagal untuk persiapan menerima koneksi, keluar program
        printError("Gagal mempersiapkan node untuk menerima koneksi! Keluar program, node berhenti.\n"); 
    }

    //Variabel socklen_t sebagai parameter fungsi accept
    socklen_t masterLength;
    //Isi masterLength dengan besar dari masterAddress
    masterLength = sizeof(masterAddress);

    //Buff input node
    char* buff;

	//Loop agar node selalu berjalan dan menerima koneksi baru
	while (1) {
        //Menunggu hingga ada koneksi pada head queue
        //Jika ada, terima koneksi dan masterSocket dibuat
        //Bind master dengan node dan addressnya        
        if ((masterSocketFD = accept(nodeSocketFD, (struct sockaddr *) &masterAddress, &masterLength)) < 0) {
            //Jika koneksi gagal diterima, keluar program
            printError("Gagal menerima koneksi dari master! Keluar program, node berhenti.\n");
            break;
        }

        //Buat thread untuk menjalankan fungsi acceptMaster
		if (pthread_create(&connectionThread[connectionCount], NULL, acceptMaster, (void*) &masterSocketFD) < 0 ) {
            //Jika thread gagal dibuat, keluar program
			printError("Gagal membuat thread untuk menerima input dari master! Keluar program, node berhenti.\n");
			break;
		} else {
            //Jika thread berhasil dibuat, tambahkan jumlah koneksi yang sudah terbentuk
            //Tambahkan juga master socket baru ke array
            masterSocketArray[connectionCount] = masterSocketFD;
			connectionCount++;
            printf("Koneksi berhasil terbentuk!\n");
            printf("Jumlah koneksi saat ini : %d.\n", connectionCount);
            printf("\n");
		}

        //Mekanisme untuk stop node service dan close semua koneksi ke master
        //Tapi belum jalan karena ini bakal bikin multiple connection gagal
        //Nggak bakal bisa accept connection baru karena nunggu scan buff
        //Harusnya scanf sama accept masing-masing sebuah thread
        /*
        char* buff;
        scanf("%s", buff);
        buff = strtok(buff, "\t\n\r");
        if (strcmp(buff, "stop")==0) {
            for (int i=0; i<connectionCount; i++) {
                close(masterSocketArray[i]);
            }
            break;
        }
        */
	}

	//Tutup node socket 
	close(nodeSocketFD);

    //Keluar program
	return 0; 
}

void printError(const char *errMessage) {
    //Output error message
    printf("%s\n", errMessage);
    //Keluar program
    exit(1);
}

void* acceptMaster(void* socket) {
	//rawBuffer untuk menerima input message dari munin-master
    //Buffer merupakan modifikasi rawBuffer untuk diproses node
	char rawBuffer[256];
	char *buffer;

	//Mengeset seluruh char pada rawBuffer menjadi empty char -> 0
	bzero(rawBuffer, 256);

	//Loop controller, keluar ketika 0
	int control = 1;
	//Return value untuk operasi read & write ke master
	int ret = -1;
	//Menyimpan identitas master socket untuk read & write
	int masterSocketFD = *(int*) socket;

	//Mengambil dan menyimpan hostname dari node
	char nodeHostname[256];
    gethostname(nodeHostname, 255);
    //Output yang akan dikirim ke master
    char* response;

    //Membentuk string untuk dioutput
    asprintf(&response, "# munin node at %s\n", nodeHostname);
    //Output message
    if ((ret = write(masterSocketFD, response, strlen(response))) < 0) {
        //Jika gagal mengirim output ke master, keluar program
        printError("Gagal mengirim output ke master. Keluar program, node berhenti.\n");
    } else {
        //Jika berhasil, output juga di node
        printf("%s", response);
    }

	//Loop/routine untuk komunikasi client-server (master-node)
	while(control) {
		//Baca input/buff dari master
        if ((ret = read(masterSocketFD, rawBuffer, sizeof(rawBuffer))) < 0) {
            //Jika gagal membaca buff, keluar program
            printError("Gagal membaca input dari master. Keluar program, node berhenti.\n");
        } else {
            //Jika berhasil, output command di node
			printf("%s",rawBuffer);
		}

        //Mengambil token dari buffer
        buffer = strtok(rawBuffer, "\t\n\r");
        //Ambil kata pertama dari command
        int i = 0;
        while (buffer[i] != ' ') {
            if (i==strlen(buffer)) {
                break;
            } else {
                i++;
            }
        }
        //Jadikan null-terminated string
        buffer[i] = '\0';

		//Jika input message = quit
		if(strcmp(buffer, "quit")==0){
            //Kurangi jumlah koneksi
            connectionCount--;
            //Output message koneksi terputus dan jumlah koneksi pada node
            printf("Koneksi terputus!\n");
            printf("Jumlah koneksi saat ini : %d.\n", connectionCount);
            printf("\n");
            //Keluar loop
			control = 0;
			break;
		} else {
            //Jika bukan quit, proses buffer dengan fungsi handler
            messageHandler(masterSocketFD, rawBuffer, buffer, nodeHostname, i);
        }

		//Reset buff
		bzero(buffer, 256);
	}

	//Tutup master socket
	close(masterSocketFD);

	//Keluar
	return 0;
}

void messageHandler(int masterSocketFD, char* rawBuffer, char* buffer, char* nodeHostname, int i) {
    //Output yang akan dikirim ke master, dipakai jika sulit output langsung dengan write
    //Bisa menyimpan output terlebih dahulu ke response
    char* response;
    //Return value untuk operasi read & write ke master
    int ret = -1;

    if (strcmp(buffer,"cap") == 0) {
        //Output message ke master
        ret = write(masterSocketFD, "cap multigraph dirtyconfig\n", strlen("cap multigraph dirtyconfig\n"));
        //Output juga ke node
        printf("%s", "cap multigraph dirtyconfig\n");
    } else 
    if (strcmp(buffer,"nodes") == 0) {
        //Output message nama node ke master
        ret = write(masterSocketFD, strcat(nodeHostname, "\n"), strlen(nodeHostname));
        ret = write(masterSocketFD, "\n.\n", strlen("\n.\n"));
        //Output juga ke node
        printf("%s", nodeHostname);
        printf(".\n");
        //reset hostname
        gethostname(nodeHostname, 255);
    } else 
    if (strcmp(buffer, "list") == 0) {
        //Jika tidak ada kata setelah list
        if (rawBuffer[i+1] == '\n' || rawBuffer[i+1] == '\0') {
            //Output newline saja ke master
            ret = write(masterSocketFD, "\n", strlen("\n"));
            //Output newline juga ke node
            printf("\n");
        } else {
            //Jika ada kata setelah list
            //Ambil kata tersebut, taruh di nextBuffer
            int j = 0;
            //Ambil address setelah token kata pertama
            char *nextBuffer = &buffer[i+1];
            //Iterasi hingga spasi atau null (ambil kata kedua)
            while (nextBuffer[j] != ' ') {
                if (!nextBuffer[j]) {
                    break;
                } else {
                    j++;
                }
            }
            //Jadikan null terminated string
            nextBuffer[j] = '\0';

            //Jika nextBuffer = hostname
            if (strcmp(nextBuffer,nodeHostname) == 0) {
                //Output memory
                ret = write(masterSocketFD, "memory\n", strlen("memory\n"));
                printf("memory\n");
            } else {
                //Jika tidak sama, output newline lagi
                ret = write(masterSocketFD, "\n", strlen("\n"));
                printf("\n");
            }
        }
    } else
    if (strcmp(buffer,"config") == 0) {
        //Ambil kata setelah config, taruh di nextBuffer
        int j = 0;
        //Ambil address setelah token kata pertama
        char *nextBuffer = &buffer[i+1];
        //Iterasi hingga spasi
        while (nextBuffer[j] != ' ') {
            //Berhenti jika '\0'
            if (!nextBuffer[j]) {
                break;
            } else {
                j++;
            }
        }
        //Jadikan null terminated string
        nextBuffer[j] = '\0';
        
        //Jika tidak sama dengan memory (bisa karena memang tidak ada kata)
        if (strcmp(nextBuffer,"memory") != 0) {
            //Output unknown service ke node
            ret = write(masterSocketFD, "# Unknown service\n", strlen("# Unknown service\n"));
            ret = write(masterSocketFD, ".\n", strlen(".\n"));
            //Output juga ke master
            printf("# Unknown service\n");
            printf(".\n");
        } else {    
        	//Jika kata kedua memory
        	///File stream
            FILE *freeResult;

            //Eksekusi command free dengan read mode (piping)
            //Stream return value direturn ke file stream untuk nantinya dapat dibaca hasilnya
            if (!(freeResult = popen("free", "r"))) {
            	//Jika gagal eksekusi command, exit program
                printf("Gagal mengeksekusi command 'free'. Keluar program, node berhenti.\n" );
            } else {
            	//Jika berhasil eksekusi command, parse file untuk mendapatkan 
            	//informasi mengenai total memory.
            	//Hasil file akan berbentuk seperti :
            	/*
            	              total        used        free      shared  buff/cache   available
				Mem:        3927160     1358672     1294084      303008     1274404     1998632
				Swap:             0           0           0
				*/
				//Kita perlu mengambil angka di bawah label total, yakni 3927160
				//Satuannya adalah kilobytes, oleh karena itu perlu dikalikan dengan 1024 nantinya
				//agar mendapatkan hasil dalam satuan bytes

				//Char untuk menampung used dan free memory
                char totalMemory[256];
                //Variabel untuk menyimpan hasil file stream per line
            	char singleLine[256];

                //Dapatkan stream file line ke 2, yakni :
                //Mem:        3927160     1358672     1294084      303008     1274404     1998632
                for (int i=0; i<2; i++) {
                	fgets(singleLine, 255, freeResult);	
                }

                int c = 0, endOfLine = 0;
                //Iterasi tiap character pada line ke 2, parse used dan free memory
                while (c < strlen(singleLine) && !endOfLine) {
                	//Jika character bukan angka, lewati. Maju terus.
                	if (singleLine[c] < '0' || singleLine[c] > '9') {
                		c++; 
                	}
                    else {
                    	//Jika karakter adalah angka, artinya kita sudah berada di bawah label total.
                    	//Kita butuh angka ini, yakni 3927160. Ambil dan simpan di totalMemory
                        int k = 0;
                        while (singleLine[c] >= '0' && singleLine[c] <= '9') {
                            totalMemory[k] = singleLine[c];
                            k++;
                            c++;
                        }
						endOfLine = 1;
                    }
                }

                //Tutup piping
                pclose(freeResult);
               
               	//Ubah totalMemory ke tipe long long integer, agar tidak overflow
                long long totalValue = atoi(totalMemory);
                //Convert ke bytes
                totalValue *= 1024;
                //Susun string
                asprintf(&response, "graph_args --base 1024 -l 0 --upper-limit %lld\n", totalValue);
                //Output ke master
                ret = write(masterSocketFD, response, strlen(response));
                //Output ke node juga
                printf("%s\n", response);
            }

            //Output ke master
            ret = write(masterSocketFD, "graph_vlabel Bytes\n", strlen("graph_vlabel Bytes\n"));
            ret = write(masterSocketFD, "graph_title Memory usage\n", strlen("graph_title Memory usage\n"));
            ret = write(masterSocketFD, "graph_category system\n", strlen("graph_category system\n"));
            ret = write(masterSocketFD, "graph_info This graph shows this machine memory.\n", strlen("graph_info This graph shows this machine memory.\n"));
            ret = write(masterSocketFD, "graph_order used free\n", strlen("graph_order used free\n"));
            ret = write(masterSocketFD, "used.label used\n", strlen("used.label used\n"));
            ret = write(masterSocketFD, "used.draw STACK\n", strlen("used.draw STACK\n"));
            ret = write(masterSocketFD, "used.info Used memory.\n", strlen("used.info Used memory.\n"));
            ret = write(masterSocketFD, "free.label free\n", strlen("free.label free\n"));
            ret = write(masterSocketFD, "free.draw STACK\n", strlen("free.draw STACK\n"));
            ret = write(masterSocketFD, "free.info Free memory.\n", strlen("free.info Free memory.\n"));
            ret = write(masterSocketFD, ".\n", strlen(".\n"));

            //Output ke node
            printf("graph_vlabel Bytes\n");
            printf("graph_title Memory usage\n");
            printf("graph_category system\n");
            printf("graph_info This graph shows this machine memory.\n");
            printf("graph_order used free\n");
            printf("used.label used\n");
            printf("used.draw STACK\n");
            printf("used.info Used memory.\n");
            printf("free.label free\n");
            printf("free.draw STACK\n");
            printf("free.info Free memory.\n");
            printf(".\n");
        }
    } else 
    if (strcmp(buffer,"fetch") == 0) {
        //Ambil kata setelah config, taruh di nextBuffer
        int j = 0;
        //Ambil address setelah token kata pertama
        char *nextBuffer = &buffer[i+1];
        //Iterasi hingga spasi (ambil kata kedua)
        while (nextBuffer[j] != ' ') {
            //Berhenti jika '\0'
            if (!nextBuffer[j]) {
                break;
            } else {
                j++;
            }
        }
        //Jadikan null terminated string
        nextBuffer[j] = '\0';
        
        //Jika tidak sama dengan memory (bisa karena memang tidak ada kata)
        if (strcmp(nextBuffer,"memory") != 0) {
            //Output unknown service ke node
            ret = write(masterSocketFD, "# Unknown service\n", strlen("# Unknown service\n"));
            ret = write(masterSocketFD, ".\n", strlen(".\n"));
            //Output juga ke master
            printf("# Unknown service\n");
            printf(".\n");
        } else {
        	//Jika kata kedua memory
        	//File stream
            FILE *freeResult;

            //Eksekusi command free dengan read mode (piping)
            //Stream return value direturn ke file stream untuk nantinya dapat dibaca hasilnya
            if (!(freeResult = popen("free", "r"))) {
            	//Jika gagal eksekusi command, exit program
                printf("Gagal mengeksekusi command 'free'. Keluar program, node berhenti.\n" );
            } else {
            	//Jika berhasil eksekusi command, parse file untuk mendapatkan 
            	//informasi mengenai used memory dan free memory.
            	//Hasil file akan berbentuk seperti :
            	/*
            	              total        used        free      shared  buff/cache   available
				Mem:        3927160     1358672     1294084      303008     1274404     1998632
				Swap:             0           0           0
				*/
				//Kita perlu mengambil angka di bawah label used dan free, yakni 1358672 dan 1294084
				//Satuannya adalah kilobytes, oleh karena itu perlu dikalikan dengan 1024 nantinya
				//agar mendapatkan hasil dalam satuan bytes

				//Char untuk menampung used dan free memory
                char usedMemory[256];
                char freeMemory[256];
                //Variabel untuk menyimpan hasil file stream per line
            	char singleLine[256];

                //Dapatkan stream file line ke 2, yakni :
                //Mem:        3927160     1358672     1294084      303008     1274404     1998632
                for (int i=0; i<2; i++) {
                	fgets(singleLine, 255, freeResult);	
                }

                int c = 0, pos = 0, endOfLine = 0;
                //Iterasi tiap character pada line ke 2, parse used dan free memory
                while (c < strlen(singleLine) && !endOfLine) {
                	//Jika character bukan angka, lewati. Maju terus.
                	if (singleLine[c] < '0' || singleLine[c] > '9') {
                		c++; 
                	}
                    else {
                    	//Jika karakter adalah angka, periksa pos
                        int k = 0;
                        switch (pos) {
                        	//Jika pos = 0, artinya angka ada di bawah label 'total', yakni : 3927160
                        	//Kita tidak butuh angka ini, lewati
                        	case 0: 
	                        	while (singleLine[c] >= '0' && singleLine[c] <= '9') {
	                                c++;
	                            }
	                            pos++;
	                            break;
	                        //Jika pos = 1, artinya angka ada di bawah label 'used', yakni : 1358672
                        	//Kita butuh angka ini, ambil dan simpan di usedMemory
	                        case 1:
	                        	while (singleLine[c] >= '0' && singleLine[c] <= '9') {
	                                usedMemory[k] = singleLine[c];
	                                k++;
	                                c++;
	                            }
	                            pos++;
	                            break;
	                        //Jika pos = 2, artinya angka ada di bawah label 'free', yakni : 1294084
                        	//Kita butuh angka ini, ambil dan simpan di freeMemory
	                        case 2:
	                        	while (singleLine[c] >= '0' && singleLine[c] <= '9') {
		                            freeMemory[k] = singleLine[c];
		                            k++;
		                            c++;
		                        }    
		                        pos++;
		                        break;
		                    //Jika pos = 3, artinya tidak ada lagi informasi yang dibutuhkan 
		                    //dari line ke 2. Berhenti iterasi.
		                    case 3:
								endOfLine = 1;
		                    	break;
                        }            
                    }
                }

                //Tutup piping
                pclose(freeResult);

                //Ubah usedMemory ke tipe integer
                int usedVal = atoi(usedMemory);
                //Convert ke bytes, ubah tipe ke long long integer agar tidak overflow
                long long usedValue = usedVal * 1024;
                //Susun string
                asprintf(&response, "used.value %lld\n", usedValue);
                //Write ke master
                ret = write(masterSocketFD, response, strlen(response));
                //Write ke node juga
                printf("%s", response);


                //Ubah freeMemory ke tipe long long integer, jaga-jaga agar tidak overflow
                int freeVal = atoi(freeMemory);
                //Convert ke bytes, ubah tipe ke long long integer agar tidak overflow
                long long freeValue = freeVal * 1024;
                //Susun string
                asprintf(&response, "free.value %lld\n", freeValue);
                //Write ke master
                ret = write(masterSocketFD, response, strlen(response));
                ret = write(masterSocketFD, ".\n", strlen(".\n"));
                //Write ke node juga
                printf("%s.\n", response);                
            }
        }
    } else 
    if (strcmp(buffer,"version") == 0) {
        //Output munins node & version 
        ret = write(masterSocketFD, "munins node on ", strlen("munins node on "));
        ret = write(masterSocketFD, nodeHostname, strlen(nodeHostname));
        ret = write(masterSocketFD, " version: 2.0.25-2\n ", strlen("version: 2.0.25-2\n "));
        //Output juga ke master
        printf("munins node on %s version: 2.0.25-2\n", nodeHostname);
        gethostname(nodeHostname, 255);
    } 
    else {
    	//Output pesan error ke master
        ret = write(masterSocketFD, "# Unknown command. Try cap, list, nodes, config, fetch, version or quit\n", strlen("# Unknown command. Try cap, list, nodes, config, fetch, version or quit\n"));
        //Output ke node juga
        printf("# Unknown command. Try cap, list, nodes, config, fetch, version or quit\n");
    }

    if (ret < 0) {
        //Jika gagal mengirim output ke master, keluar program
        printError("Gagal mengirim output ke master. Keluar program, node berhenti.\n");
    }
}