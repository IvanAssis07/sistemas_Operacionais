# PAGINADOR DE MEMÓRIA - RELATÓRIO

1. Termo de compromisso

Os membros do grupo afirmam que todo o código desenvolvido para este
trabalho é de autoria própria.  Exceto pelo material listado no item
3 deste relatório, os membros do grupo afirmam não ter copiado
material da Internet nem ter obtido código de terceiros.

2. Membros do grupo e alocação de esforço

Preencha as linhas abaixo com o nome e o e-mail dos integrantes do
grupo.  Substitua marcadores `XX` pela contribuição de cada membro
do grupo no desenvolvimento do trabalho (os valores devem somar
100%).

  * Caio Teles Cunha <caiotelescunha2001@gmail.com> 50%
  * Ivan Vilaça de Assis <ivan.assis07@gmail.com> 50%

3. Referências bibliográficas
  https://pubs.opengroup.org/onlinepubs/7908799/xsh/ucontext.h.html
  https://www.ibm.com/docs/en/i/7.2?topic=ssw_ibm_i_72/apis/setitime.html
  https://cboard.cprogramming.com
  https://stackoverflow.com
  https://youtu.be/E45WeZIg0aY

  Slides e vídeos gravados da disciplina no moodle.

4. Estruturas de dados

  1. Descreva e justifique as estruturas de dados utilizadas para
     gerência das threads de espaço do usuário (partes 1, 2 e 5).

     Para as partes 1 e 2, foi preciso definir uma struct para a dccthread, essa struct possuia inicialmente um atributo name (Para identificar a thread) e um atributo ucontext_t (usado para guardar os dados referentes ao contexto de execução da thread). Inicializamos um ponteiro para uma thread manager de forma global para poder acessar facilmente essa thread de todo o código. Inicializamos também uma fila duplamente encadeada de threads para guardar as threads criadas, pois essa estrutura mantém a ordem das threads que serão executadas e da forma como foi implementada podemos acessar em O(1) a cabeça e a cauda da lista, o que facilita e torna mais rápido as operações de inserção e remoção das threads na lista que são muito utilizadas ao longo do código. A thread manager possui uma função com o objetivo de escalonar corretamente as threads que foram criadas a partir da main.

     Para a parte 5, acrescentamos um ponteiro para uma thread na struct dccthread, esse ponteiro aponta para a thread que cada thread está esperando(caso exista), foi implementado dessa forma devido à especificação que cada thread espera somente uma outra. Também acrescentamos uma nova fila para armazenar as threads dormindo, pelos mesmos motivos da fila de cima. Essa fila separa as threads dormindo das threads prontas evitando que a manager coloque-as em execução. Preferimos fazer dessa forma para evitar aumentar atributos na struct dccthread e facilitar as comparações que são feitas na manager. A criação dessa thread também facilita a compreensão do código e seu desenvolvimento.

  2. Descreva o mecanismo utilizado para sincronizar chamadas de
     dccthread_yield e disparos do temporizador (parte 4).

    Para evitar que o sinal do timer fosse disparado e que a ação de chamar o yield ocorrece, usamos a função 'sigprocmask' no início e no final das funções que alteravam as estruturas de dados da nossa biblioteca para bloquear e desbloquear o sinal respectivamente.

    Um ponto que tivemos que nos atentar foi que a Thread Manager não pode estar sujeita aos disparos de sinais, já que ela é a thread chamada pelo yield() e a responsável por gerenciar todas as outras. 

    Então, bloqueamos o sinal do disparo logo antes de executar a thread gerente  e salvamos este estado de bloqueio usando o campo 'uc_sigmask' do 'context' da nossa Thread gerente. Com isso, garantimos que todas as vezes que estivemos na thread manager o disparado de sinal do temporizador está bloqueado e o handler que chama o yield() não será chamado.

    Também tivemos que nos atentar ao sinal de Threads que são criadas. Não queremos criar uma Thread e ela já ter o sinal do temporizador bloqueado, por isso utilizamos o campo 'uc_sigmask' do 'context', mas dessa vez junto com a função 'sigemptyset()' que faz com que a máscara de sinal de uma Thread seja vazia, garantindo que nenhum sinal está bloqueado inicialmente para aquela nova Thread. Isso permite que uma nova Thread esteja aberta para receber o sinal de disparo do temporizador e execute o handler.
