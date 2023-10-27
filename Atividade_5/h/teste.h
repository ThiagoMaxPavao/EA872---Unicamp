	/* fecha stdin - proxima chamada open tera' descritor igual a 0 */
	fd1 = open("h.c", O_RDONLY, 0600);
	  perror("open fd1");
	/* fecha stdout - proxima chamada open tera' descritor igual a 1 */
	fd2 = open("./teste.h", O_WRONLY | O_CREAT, 0600);
	  perror("open fd2");
	system("grep \"open\"");
