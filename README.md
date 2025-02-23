Joc șah client-server TCP
      - Se va implementa jocul după regulile standard de șah
          - Componenta SERVER: 
              o va implementa efectiv logica de joc 
              o va aștepta conexiuni de la jucători ce se conectează prin componenta client
              o va conecta tot câte doi jucători și va gestiona jocul
              o va gestiona mai multe jocuri în paralel
              o după fiecare meci va permite continuarea jocului cu un meci nou folosind aceeași 
              structură de jucători
              o va identifica jucătorii printr-un nume
          - Componenta CLIENT:
              o Va implementa componenta de interfațare cu utilizatorul
              o În varianta minimală se va afișa tabla de joc în mod text
              o Va aștepta asignarea unui jucător partener de către componenta server
              o Nu va implementa deloc logica jocului, ci va fi doar un client de interfațare, logica efectivă 
              a jocului fiind implementată doar de server
