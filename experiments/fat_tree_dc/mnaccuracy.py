import numpy as np 
import matplotlib.pyplot as plt
from matplotlib import container

def read_file(fname, k, measurements):
	measurements[k] = []
	with file(fname, "r") as f:
		lines = f.readlines()
		for line in lines:
			line = line.strip('\n')
			pts = [float(x) for x in line[1:-1].split(",")]
			measurements[k].append(pts[0:52])	


def write_file(fname, mean_data, std_data):
	with file(fname, "w") as f:
		f.write("# Rate(Mbps) Time(s) Error\n")
		for i, mean in enumerate(zip(mean_data, std_data)):
			f.write("%s %s %s\n" % (mean[0], i, mean[1]))


measurements = {}
read_file("k-4", 4, measurements)
read_file("k-6", 6, measurements)
read_file("k-8", 8, measurements)


k4 = np.array(measurements[4])
k6 = np.array(measurements[6])
k8 = np.array(measurements[8])

k4mean = np.mean(k4, axis=0)
k4std = np.std(k4, axis=0)

k6mean = np.mean(k6, axis=0)
k6std = np.std(k6, axis= 0)

k8mean = np.mean(k8, axis=0)
k8std = np.std(k8, axis = 0)


time = [x for x in range(0,52)]
expct4 = [16 for x in range(1, 52)]
expct4.insert(0, 0)
expct6 = [54 for x in range(1, 52)]
expct6.insert(0, 0)
expct8 = [128 for x in range(1, 52)]
expct8.insert(0, 0)


fig, axs = plt.subplots(nrows=3, ncols=1, sharex=True, sharey=True)

ax = axs[0]
# ax.plot(time,k4mean, label='Results')

# Removes the error lines of the legend
# handles, labels = ax.get_legend_handles_labels()
# handles = [h[0] if isinstance(h, container.ErrorbarContainer) else h for h in handles]

ax.errorbar(time, k4mean, yerr=k4std, errorevery=5, label= "Result")
ax.plot(time, expct4, 'r--', linewidth = 4, label="Expected")
ax.set_title('Fat Tree with k = 4')
handles, labels = ax.get_legend_handles_labels()
handles = [h[0] if isinstance(h, container.ErrorbarContainer) else h for h in handles]
ax.legend(handles, labels, loc='best', prop={'size': 10})
ax.set(ylabel='Rate (Mbps)')

ax = axs[1]
ax.errorbar(time, k6mean, yerr=k6std, errorevery=5,  label="Result")
ax.plot(time, expct6, 'r--', linewidth = 4,  label="Expected")
ax.set_title('Fat Tree with k = 6')
ax.legend(loc='best', prop={'size': 10})

handles, labels = ax.get_legend_handles_labels()
handles = [h[0] if isinstance(h, container.ErrorbarContainer) else h for h in handles]
ax.legend(handles, labels, loc='best', prop={'size': 10})
ax.set(ylabel='Rate (Mbps)')

ax = axs[2]
ax.errorbar(time, k8mean, yerr=k8std, errorevery=5, label= "Result")
ax.plot(time, expct8, 'r--', linewidth = 4, label="Expected")
ax.set_title('Fat Tree with k = 8')
ax.legend(loc='best', prop={'size': 10})
handles, labels = ax.get_legend_handles_labels()
handles = [h[0] if isinstance(h, container.ErrorbarContainer) else h for h in handles]
ax.legend(handles, labels, loc='best', prop={'size': 10})
ax.set(xlabel='Time (s)', ylabel='Rate (Mbps)')
# write_file("k4.dat", k4mean, k4std)
# write_file("k6.dat", k6mean, k6std)
# write_file("k8.dat", k8mean, k8std)


fig.suptitle('Accuracy of Mininet as the size of the topology grows')

plt.show()

