#include <bits/stdc++.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <openssl/md5.h>
#define BUFFSIZE 16384

using namespace std;
#define timeout 2

vector<string> files_of_r_dir;
int r_port = 0;
int r_uniqueid = 0;

struct connection_data{
    string data;
    int clientid;
};

bool compareFunctionp(struct connection_data a, struct connection_data b) 
{
    return a.clientid<b.clientid;
} 

bool compareFunction (std::string a, std::string b) 
{
    return a<b;
} 

struct neb_files_data
{
    string filename;
    string port_no;
    string uniqueid;
};

string get_md5hash(const string &fname)
{

    char buffer[BUFFSIZE];
    unsigned char digest[MD5_DIGEST_LENGTH];

    stringstream ss;
    string md5string;

    ifstream ifs(fname, ifstream::binary);

    MD5_CTX md5Context;

    MD5_Init(&md5Context);

    while (ifs.good())
    {

        ifs.read(buffer, BUFFSIZE);

        MD5_Update(&md5Context, buffer, ifs.gcount());
    }

    ifs.close();

    int res = MD5_Final(digest, &md5Context);

    if (res == 0)  
    return {}; 

    ss << hex << uppercase << setfill('0');

    for (unsigned char uc : digest)
        ss << setw(2) << (int)uc;

    md5string = ss.str();

    return md5string;
}

void ClientsforAllServers(int *idwithport[2], int nebcount, int no_of_files, string *filesname, string ddir)
{

    int sockfd[nebcount];
    struct sockaddr_in servaddr[nebcount];
    bool connected[nebcount];
    int connections = 0;
    string id = "";
    string uniqueid = "";

    for (int i = 0; i < nebcount; i++)
    {
        sockfd[i] = socket(AF_INET, SOCK_STREAM, 0);
        connected[i] = false;
        if (sockfd[i] == -1)
        {
            perror("socket creation failed");
            exit(0);
        }
        servaddr[i].sin_family = AF_INET;
        servaddr[i].sin_addr.s_addr = INADDR_ANY;
        servaddr[i].sin_port = htons(idwithport[1][i]);
        memset(&(servaddr[i].sin_zero), '\0', 8);
    }

    vector<struct connection_data> data_to_print;
	while(true){
        if(connections==nebcount) break;
        for (int neb=0; neb<nebcount; neb++)
        {
            if(!connected[neb]){
                if (connect(sockfd[neb], (struct sockaddr *)&servaddr[neb], sizeof(servaddr[neb])) < 0) {
                    continue;
                }
                else{
                    struct connection_data temp;
                    connections++;
                    connected[neb] = true;
                    char buff[100];
                    recv(sockfd[neb], buff, 99, 0);

                    int indexofdash[2] = {0};
                    int ap=0;
                    for(int i=0; i<90; i++){
                        if(buff[i] == '-') {
                            indexofdash[ap] = i;
                            ap++;
                        }
                    }
                    for(int i=0; i< indexofdash[0]; i++){
                        id.push_back (buff[i]);
                    }
                    for(int i=indexofdash[0]+1; i< indexofdash[1]; i++){
                        uniqueid.push_back(buff[i]);                        
                    }

                    int pport =0;
                    for(int i=0; i< nebcount; i++){
                        if(idwithport[0][i] == int(id[0] - '0')  ){
                            pport = idwithport[1][i];
                        }
                    }
                    temp.data = "Connected to " + id+ " with unique-ID "+ uniqueid +" on port "+ to_string(pport);
                    temp.clientid = stoi(id);
                    data_to_print.push_back(temp);
                    id ="";uniqueid="";pport =0;indexofdash[0]=0; indexofdash[1] = 0;
                }
            }
        }
    }
    sort(data_to_print.begin(),data_to_print.end(),compareFunctionp);//sort the vector
    for(auto dat : data_to_print){
        cout << dat.data << endl;
    }

    for (auto file : files_of_r_dir)
    {
        for (int j = 0; j < nebcount; j++)
        {
            if (file[0] == '.')
                continue;
            
            string p = "";
            p.push_back(3);
            p= p+file + "-" + to_string(r_uniqueid) + "-" + to_string(r_port) + "-";
            char help[p.length() + 1] = " ";
            for (int h = 0; h < p.length(); h++)
            {
                help[h] = p[h];
            }
            help[p.length()] = '\0';
            send(sockfd[j], help, sizeof(help) + 1, 0);
            p.clear();
        }
        sleep(0.05);
    }

    sleep(0.1);
    for (int j = 0; j < nebcount; j++)
    {
        string al ="";
        al.push_back(4); al.push_back(3);
        string es = al + "allfilessended";
        char helpp[es.length() + 1] = " ";
        for (int h = 0; h < es.length(); h++)
        {
            helpp[h] = es[h];
        }
        helpp[es.length()] = '\0';
        send(sockfd[j], helpp, sizeof(helpp) + 1, 0);
        es.clear();
    }

    sleep(0.1);

    //----------------
    fd_set master;
    fd_set read_fds;
    int fdmax;
    int newfd;
    int addrlen;
    int nebcp = 0;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    fdmax = sockfd[0];
    for (int i = 0; i < nebcount; i++)
    {
        FD_SET(sockfd[i], &master);
        if (sockfd[i] > fdmax)
            fdmax = sockfd[i];
    }

    char buff[100];
    int fd_uid[no_of_files] = {0}, fd_port[no_of_files] = {0}, fd_depth[no_of_files] = {0};
    int newsocks[no_of_files];
    for (int i = 0; i < no_of_files; i++)
    {
        newsocks[i] == -1;
    }

    for (int i = 0; i < no_of_files; i++)
    {
        int fd_uid1 = 0, fd_port1 = 0, fd_depth1 = 0;
        int cli = 0;
        int depth = 0;
        char f[filesname[i].length() + 1] = " ";

        for (int j = 0; j < filesname[i].length(); j++)
        {
            f[j] = filesname[i][j];
        }
        f[filesname[i].length()] = '\0';

        for (int j = 0; j < nebcount;)
        {
            send(sockfd[j], f, sizeof(f) + 1, 0);
            read_fds = master;
            struct timeval tv;
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            if (select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1)
            {
                perror("select");
                exit(1);
            }
            if (FD_ISSET(sockfd[j], &read_fds))
            {
                int ps = sockfd[j];
                recv(sockfd[j], buff, 99, 0);
                j++;

                int pos_d[3];
                int ind = 0;
                for (int al = 0; al < 40; al++)
                {
                    if (buff[al] == '-')
                    {
                        pos_d[ind] = al;
                        ind++;
                    }
                }
                string temp;
                for (int al = 0; al < pos_d[0]; al++)
                {
                    temp.push_back(buff[al]);
                }
                fd_uid1 = stoi(temp);
                if (fd_uid1 == 0)
                    continue;
                temp.clear();
                for (int al = pos_d[0] + 1; al < pos_d[1]; al++)
                {
                    temp.push_back(buff[al]);
                }
                fd_depth1 = stoi(temp);
                temp.clear();
                for (int al = pos_d[1] + 1; al < pos_d[2]; al++)
                {
                    temp.push_back(buff[al]);
                }
                fd_port1 = stoi(temp);

                if (fd_depth[i] == 0)
                {
                    fd_uid[i] = fd_uid1;
                    fd_depth[i] = fd_depth1;
                    fd_port[i] = fd_port1;
                }

                else if (fd_depth1 == 1 and fd_depth[i] == 2)
                {
                    fd_uid[i] = fd_uid1;
                    fd_depth[i] = fd_depth1;
                    fd_port[i] = fd_port1;
                }
                else if (fd_depth1 == 1 and fd_depth[i] == 1)
                {
                    if (fd_uid[i] > fd_uid1)
                    {
                        fd_uid[i] = fd_uid1;
                        fd_port[i] = fd_port1;
                    }
                }
                else if (fd_depth1 == 2 and fd_depth[i] == 2)
                {
                    if (fd_uid[i] > fd_uid1)
                    {
                        fd_uid[i] = fd_uid1;
                        fd_port[i] = fd_port1;
                    }
                }
            }
        }
    }

    struct sockaddr_in nservaddr[no_of_files];
    bool nconnected[no_of_files] = {false};
    int nconnections = 0;

    for (int i = 0; i < no_of_files; i++)
    {
        nconnected[i] = false;
        newsocks[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (newsocks[i] == -1)
        {
            perror("socket creation failed");
            exit(0);
        }
        nservaddr[i].sin_family = AF_INET;
        nservaddr[i].sin_addr.s_addr = INADDR_ANY;
        nservaddr[i].sin_port = htons(fd_port[i]);

        memset(&(nservaddr[i].sin_zero), '\0', 8);
    }
    bool download_directory_created = false;
    bool sent[no_of_files] = {false};

    int fi = 0;
    for (int i = 0; i < no_of_files; i++)
    {
        if (fd_uid[i] != 0)
            fi++;
    }

    while (fi > 0)
    {
        for (int neb = 0; neb < no_of_files; neb++)
        {

            if (!nconnected[neb] and fd_uid[neb] != 0)
            {
                if (connect(newsocks[neb], (struct sockaddr *)&nservaddr[neb], sizeof(nservaddr[neb])) < 0)
                {
                    continue;
                }
                else
                {
                    nconnected[neb] = true;
                    nconnections++;
                    char buff[100];
                    recv(newsocks[neb], buff, 99, 0);

                    fi--;
                    if (!download_directory_created)
                    {
                        mkdir((ddir + "Downloaded/").c_str(), 0777);
                    }

                    char s[filesname[neb].length() + 3] = " ";
                    s[0] = '!';
                    s[1] = '!';
                    for (int j = 2; j < filesname[neb].length() + 2; j++)
                    {
                        s[j] = filesname[neb][j - 2];
                    }
                    s[filesname[neb].length() + 2] = '\0';
                    send(newsocks[neb], s, sizeof(s) + 1, 0);
                    int n = 0;
                    char buf[100];
                    int siz = 0;
                    if ((n = recv(newsocks[neb], buf, 100, 0) < 0))
                    {
                        perror("recv_size()");
                        exit(errno);
                    }
                    siz = atoi(buf);
                    int size = siz;
                    int stpt = 0;
                    char Rbuffer[siz];
                    n = 0;
                    while (size > 0)
                    {
                        if ((n = recv(newsocks[neb], Rbuffer + stpt, siz, 0)) < 0)
                        {
                            perror("recv_size()");
                            exit(errno);
                        }
                        else
                        {
                            size = size - n;
                            stpt = stpt + n;
                        }
                    }

                    FILE *filerecived;
                    filerecived = fopen((ddir + "Downloaded/" + filesname[neb]).c_str(), "w");
                    fwrite(Rbuffer, sizeof(char), siz, filerecived);
                    fclose(filerecived);
                    // }
                }
            }
        }
    }
    for (int i = 0; i < no_of_files; i++)
    {
        if (fd_uid[i] != 0)
        {
            string s1 = (ddir + "Downloaded/" + filesname[i]);
            string s2 = get_md5hash(s1);
            transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
            cout << "Found " + filesname[i] + " at " << fd_uid[i] << " with MD5 " << s2 << " at depth " << fd_depth[i] << endl;
        }
        else
        {
            cout << "Found " + filesname[i] + " at " << fd_uid[i] << " with MD5 0 at depth " << fd_depth[i] << endl;
        }
    }
}

void ServerforAllClients(int sport, int *idwithport[2], int nebcount, int selfid, int selfuniqueid, vector<string> files, string ourdirectory)
{
    string id_data = to_string(selfid) + "-" + to_string(selfuniqueid) + "-";
    int k = id_data.length();
    char id_datap[k] = " ";
    int no_of_nebs_done = 0;
    for (int i = 0; i < k; i++)
    {
        id_datap[i] = id_data[i];
    }

    vector<neb_files_data> neb_f_data;
    char *buff = new char[100];
    fd_set master;
    fd_set read_fds;
    struct sockaddr_in myaddr;
    struct sockaddr_in remoteaddr;
    int fdmax;
    int listener;
    int newfd;
    char buf[256];
    int nbytes;
    int yes = 1;
    int addrlen;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    myaddr.sin_port = htons(sport);
    memset(&(myaddr.sin_zero), '\0', 8);
    if (bind(listener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }
    if (listen(listener, 10) < 0)
    {
        perror("listen");
        exit(1);
    }
    FD_SET(listener, &master);
    fdmax = listener;
    for (;;)
    {

        read_fds = master;
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        if (select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1)
        {
            perror("select");
            exit(1);
        }
        for (int i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == listener)
                {
                    addrlen = sizeof(remoteaddr);
                    if ((newfd = accept(listener, (struct sockaddr *)&remoteaddr, (socklen_t *)&addrlen)) == -1)
                    {
                        perror("accept");
                    }
                    else
                    {
                        FD_SET(newfd, &master);
                        if (newfd > fdmax)
                            fdmax = newfd;
                        send(newfd, id_datap, sizeof(id_datap) + 1, 0);
                    }
                }
                else
                {
                    char bufp[100];
                    recv(i, bufp, 99, 0);

                    if (bufp[0] == 4 and bufp[1] == 3)
                    {
                        no_of_nebs_done++;
                    }
                    else if (bufp[0] == '!' and bufp[1] == '!')
                    {
                        string filenametosend;
                        for (int i = 2; i < 99; i++)
                        {
                            filenametosend.push_back(bufp[i]);
                        }
                        FILE *filetosend;
                        char buf[50] = " ";
                        string dir = ourdirectory + filenametosend;
                        filetosend = fopen(dir.c_str(), "r");
                        fseek(filetosend, 0, SEEK_END);
                        int siz = ftell(filetosend);
                        int n;

                        sprintf(buf, "%d", siz);
                        if ((n = send(i, buf, sizeof(buf), 0)) < 0)
                        {
                            perror("send_size()");
                            exit(errno);
                        }
                        sleep(0.05);
                        char Sbuf[siz] = " ";
                        fseek(filetosend, 0, SEEK_END);
                        siz = ftell(filetosend);
                        fseek(filetosend, 0, SEEK_SET);
                        while (!feof(filetosend))
                        {
                            n = fread(Sbuf, sizeof(char), siz, filetosend);
                            if (n > 0)
                            { 
                                if ((n = send(i, Sbuf, siz, 0)) < 0)
                                {
                                    perror("send_data()");
                                    exit(errno);
                                }
                            }
                            sleep(0.005);
                        }
                    }

                    else if (bufp[0] == 3 )
                    {
                        struct neb_files_data temp;
                        int pod[3];
                        int hp = 0;
                        int indexer = 0;
                        while (hp != 3)
                        {
                            if (bufp[indexer] == '-')
                            {
                                pod[hp] = indexer;
                                hp++;
                            }
                            indexer++;
                        }
                        for (hp = 1; hp < pod[0]; hp++)
                        {
                            temp.filename.push_back(bufp[hp]);
                        }
                        for (hp = pod[0] + 1; hp < pod[1]; hp++)
                        {
                            temp.uniqueid.push_back(bufp[hp]);
                        }
                        for (hp = pod[1] + 1; hp < pod[2]; hp++)
                        {
                            temp.port_no.push_back(bufp[hp]);
                        }
                        neb_f_data.push_back(temp);
                    }
                    else
                    {
                        if (no_of_nebs_done != nebcount)
                            continue;
                        bool p = false;
                        for (auto file : files)
                        {
                            for (int i = 0; i < sizeof(file); i++)
                            {
                                if (file[i] == '\0')
                                {
                                    p = true;
                                    break;
                                }
                                if (file[i] != bufp[i])
                                    break;
                            }
                        }

                        if (p)
                        {
                            string as = to_string(selfuniqueid) + "-1-" + to_string(sport) + "-";
                            send(i, as.c_str(), sizeof(as.c_str()) + 4, 0);
                        }
                        else
                        {
                            bool pp = false;
                            struct neb_files_data tp;
                            tp.uniqueid = "0";
                            int sd = 0;

                            for (auto file_d : neb_f_data)
                            {
                                for (int i = 0; i < sizeof(file_d.filename); i++)
                                {
                                    if (file_d.filename[i] == '\0')
                                    {
                                        pp = true;
                                        if ((sd == 0) or (stoi(file_d.uniqueid) < sd))
                                        {
                                            if (stoi(tp.uniqueid) == 0 or (stoi(file_d.uniqueid) < stoi(tp.uniqueid)))
                                            {
                                                tp = file_d;
                                            }
                                        }
                                        break;
                                    }
                                    if (file_d.filename[i] != bufp[i])
                                        break;
                                }
                            }
                            if (pp)
                            {
                                string a = tp.uniqueid + "-2-" + tp.port_no + "-";
                                send(i, a.c_str(), sizeof(a.c_str()) + 4, 0);
                            }
                            else
                            {
                                send(i, "0000-\0", 7, 0);
                            }
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{

    // printing files of client's directory -----------------------------------------
    DIR *dir;
    struct dirent *diread;
    vector<string> files;

    if ((dir = opendir(argv[2])) != nullptr)
    {
        while ((diread = readdir(dir)) != nullptr)
        {
            files.push_back(diread->d_name);
        }
        closedir(dir);
    }
    else
    {
        perror("opendir");
        return EXIT_FAILURE;
    }
    sort(files.begin(),files.end(),compareFunction);//sort the vector
    for (auto file : files)
    {
        if (file[0] == '.')
            continue;
        files_of_r_dir.push_back(file);
        cout << file << endl;
    }

    //---------------------------------------------------------------------------

    // Reading configuration file-----------------------------------------------
    int count = 0;
    int selfid = 0, selfport = 0;
    int selfuniqueid = 0;
    int nebcount = 0;
    int **idwithport = new int *[2];
    int no_of_files = 0;
    string *filesname;
    ifstream fin;
    string lineText;
    fin.open(argv[1]);
    while (fin)
    {
        getline(fin, lineText);
        if (count == 0)
        {
            stringstream firstline(lineText);
            firstline >> selfid >> selfport >> selfuniqueid;
            r_port = selfport;
            r_uniqueid = selfuniqueid;
        }
        else if (count == 1)
        {
            stringstream firstline(lineText);
            firstline >> nebcount;
        }
        else if (count == 2)
        {
            idwithport[0] = new int[nebcount];
            idwithport[1] = new int[nebcount];
            stringstream firstline(lineText);
            for (int i = 0; i < nebcount; i++)
            {
                firstline >> idwithport[0][i] >> idwithport[1][i];
            }
        }
        else if (count == 3)
        {
            stringstream firstline(lineText);
            firstline >> no_of_files;
            filesname = new string[no_of_files];
        }
        else
        {
            for (int i = 0; i < no_of_files; i++)
            {
                stringstream firstline(lineText);
                firstline >> filesname[i];
                if (i == no_of_files - 1)
                    break;
                getline(fin, lineText);
            }
            break;
        }
        count++;
    }
    fin.close();

    //-------------------------------------------------------------
    vector<string> temp;
    for(int i=0; i<no_of_files; i++){
        temp.push_back(filesname[i]);
    }
    sort(temp.begin(),temp.end(),compareFunction);//sort the vector
    for(int i=0; i<no_of_files; i++){
        filesname[i]= temp[i];
    }
    //--------------------------------------------------------------

    thread t1(ServerforAllClients, selfport, idwithport, nebcount, selfid, selfuniqueid, files, string(argv[2]));
    thread t2(ClientsforAllServers, idwithport, nebcount, no_of_files, filesname, string(argv[2]));
    t1.join();
    t2.join();
}
