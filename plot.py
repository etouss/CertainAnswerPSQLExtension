import matplotlib.pyplot as plt
import numpy

w = open('q1true','r')
qstring = w.readline().split(',')
w.close()
q1 = []
for s in qstring:
    if s != '':
        q1.append(float(s))

w = open('q1dnfctrue','r')
qstring = w.readline().split(',')
w.close()
q1dnfc = []
for s in qstring:
    if s != '':
        q1dnfc.append(float(s))

w = open('q1uatrue','r')
qstring = w.readline().split(',')
w.close()
q1ua = []
for s in qstring:
    if s != '':
        q1ua.append(float(s))

w = open('q2true','r')
qstring = w.readline().split(',')
w.close()
q2 = []
for s in qstring:
    if s != '':
        q2.append(float(s))

w = open('q2dnfctrue','r')
qstring = w.readline().split(',')
w.close()
q2dnfc = []
for s in qstring:
    if s != '':
        q2dnfc.append(float(s))

w = open('q2uatrue','r')
qstring = w.readline().split(',')
w.close()
q2ua = []
for s in qstring:
    if s != '':
        q2ua.append(float(s))


q1.pop()

q1vsq1dnfc = []
for i in range(0,min(len(q1),len(q1dnfc))):
    q1vsq1dnfc.append(q1dnfc[i]/q1[i])

q2vsq2dnfc = []
for i in range(0,min(len(q2),len(q2dnfc))):
    q2vsq2dnfc.append(q2dnfc[i]/q2[i])

q2vsq2ua = []
for i in range(0,min(len(q2),len(q2ua))):
    q2vsq2ua.append(q2ua[i]/q2[i])




plt.xlabel('Database size in Go')
plt.ylabel('Evaluation time in ms')

'''
size = []
for i in range(1,len(q2ua)+1):
    size.append(i*0.5)
z = numpy.polyfit(size, q2ua, 5)
p = numpy.poly1d(z)
plt.plot(size,p(size),linewidth=2.0,label="q2ua",color="blue")
plt.plot(size, q2ua, 'bo',linewidth=0.5,color="blue")
z = numpy.polyfit(size, q2, 5)
p = numpy.poly1d(z)
plt.plot(size,p(size),linewidth=2.0,label="q2",color="green")
plt.plot(size, q2, 'bo',linewidth=0.5,color="green")
z = numpy.polyfit(size, q2dnfc, 5)
p = numpy.poly1d(z)
plt.plot(size,p(size),linewidth=2.0,label="q2dnfc",color="red")
plt.plot(size, q2dnfc, 'bo',linewidth=0.5,color="red")
plt.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc=3, ncol=3, mode="expand", borderaxespad=0.)


size = []
for i in range(1,len(q1)+1):
    size.append(i*0.5)
z = numpy.polyfit(size, q1, 4)
p = numpy.poly1d(z)
plt.plot(size,p(size),linewidth=2.0,label="q1",color="green")
plt.plot(size, q1, 'bo',linewidth=0.5,color="green")
z = numpy.polyfit(size, q1dnfc, 4)
p = numpy.poly1d(z)
plt.plot(size,p(size),linewidth=2.0,label="q1dnfc",color="red")
plt.plot(size, q1dnfc, 'bo',linewidth=0.5,color="red")
plt.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc=3, ncol=2, mode="expand", borderaxespad=0.)

'''
plt.ylabel('Relative performance')
size = []
for i in range(1,len(q2ua)+1):
    size.append(i*0.5)

z = numpy.polyfit(size, q2vsq2ua, 1)
p = numpy.poly1d(z)
plt.plot(size,p(size),linewidth=2.0,label="q2ua \ q2",color="blue")
plt.plot(size, q2vsq2ua, 'bo',linewidth=0.5,color="blue")
z = numpy.polyfit(size, q2vsq2dnfc, 1)
p = numpy.poly1d(z)
plt.plot(size,p(size),linewidth=2.0,label="q2dnfc \ q2",color="red")
plt.plot(size, q2vsq2dnfc, 'bo',linewidth=0.5,color="green")
plt.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc=3, ncol=2, mode="expand", borderaxespad=0.)
'''
plt.ylabel('Relative performance')
size = []
for i in range(1,len(q1)+1):
    size.append(i*0.5)

z = numpy.polyfit(size, q1vsq1dnfc, 5)
p = numpy.poly1d(z)
plt.plot(size,p(size),linewidth=2.0,label="q1dnfc \ q1",color="red")
plt.plot(size, q1vsq1dnfc, 'bo',linewidth=0.5,color="blue")
plt.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc=3, ncol=1, mode="expand", borderaxespad=0.)
'''

plt.show()
