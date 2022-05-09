# Come utilizzare gli scripts in test



## Preliminary directory

### generate_install.py & extract_bench.py

Questi script sono utilizzati negli script descritti di seguito, non dovrebbero essere lanciati direttamente.

### install.sh

E' uno script preliminare da utilizzare per installare i bench di phoronix. La lista dei bench è determinata automaticamente impostando i valori di runtime e download size nei filtri all'interno dello script interessato.

### install_extra.sh

E' utilizzato per installare benchmark arbitrary per sperimentazione successive.

*NOTA* Consiglio di inserire questa lista nella BLACKLIST degli script di test.

### test_false_positives.py

Testa i falsi positivi senza necessità di interazione.
Produce un log nella cartella locale.



## Generator directory

### test_threasholds.py

Testa i valori delle soglie in automatico.
Bisogna copiare manualmente l'otupout del terminale.

*NOTA* se si desidera, si possono cambiare i valori di minimo e massimo.

### evaluate_pmc_accuracy.py

Questo test va lanciato su kernel non pacthato e con perf e valgrind installati.
Testa automaticamente l'accuracy delle pmc rispetto alla controparte software.
Bisogna copiare manualmente l'otupout del terminale.

*NOTA* per migliorare l'accuraci potrebbe essere richiesto un eliminazione manuale dei massimi e minimi dalla lista.

### evaluate_plain_penalties.py

Esegue i bench definiti in BENCHMARKS in automatico, avviando le impostazioni selezionate.
L'ambiente deve essere impostato esternamente (plain, recode off, recode on ecc).
I valori vanno presi manualmente dai log generati.

### evaluate_fr_attack.py

Valuta la quantità di byte che un attacco riesce ad estrarre.
L'ambiente deve essere impostato esternamente, quindi avviare la prima sessione con recode OFF. E le successive con recode ON, il valore desiderato GAMMA e SIGNAL = 1.
Bisogna copiare manualmente l'otupout del terminale.



## Plotting directory

### plot_attack_accuracy.py

Crea i grafici dell'accuracy di un attacco nell'estrarre i dati.
Necessita come parametro la cartella contenente i dati. (*results*)

### plot_detection_accuracy.py

Crea il grafico relativo a una architettura del'accuracy.
I false negatives sono cabalati in quanto sempre individuati.
Necessita come parametro il nome del file contenente i dati.

### plot_mitigations_overhead.py

Crea il grafico delle mitigazioni dinamiche vs quelle sempre attive.
I dati sono cablati all'interno.

### plot_recode_overhead.py

Crea il  grafico degli overhead di recode vs plain system su diversi bench.
I dati sono cablati.

*Nota* I dati sono disponibili nella cartella results.

