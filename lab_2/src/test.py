import matplotlib.pyplot as plt

K = [1, 4, 6, 8, 12, 16, 128, 1024]
Sk = [1.00, 2.80, 3.26, 3.86, 4.23, 4.18, 1.67, 0.22]
Ek = [1.00, 0.70, 0.55, 0.52, 0.38, 0.27, 0.02, 0.0003]

K_ideal = [1, 4, 8, 12]

plt.figure(figsize=(10, 6))

plt.plot(K, Sk, marker='o', linestyle='-', color='blue', label='Измеренное ускорение $S_N$')

plt.title('Зависимость ускорения $S_N$ от числа потоков $N$')
plt.xlabel('Число потоков (N)')
plt.ylabel('Ускорение $S_N$')
plt.grid(True, linestyle=':', alpha=0.7)
plt.legend(loc='upper left')

plt.xticks(K)
plt.xlim(0, 20)
plt.show()

plt.figure(figsize=(10, 6))

plt.plot(K, Ek, marker='s', linestyle='-', color='green', label='Эффективность $E_N$')

plt.title('Зависимость эффективности $E_N$ от числа потоков $N$')
plt.xlabel('Число потоков (N)')
plt.ylabel('Эффективность $E_N$')
plt.grid(True, linestyle=':', alpha=0.7)
plt.legend()

plt.xticks(K)
plt.xlim(0, 20) 
plt.ylim(0, 1.05)
plt.show()