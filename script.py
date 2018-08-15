import time
import subprocess

w1 = open("q1result",'a')
w2 = open("q1uaresult",'a')
w3 = open("q1dnfcresult",'a')
w4 = open("q2result",'a')
w5 = open("q2uaresult",'a')
w6 = open("q2dnfcresult",'a')

q1 = True
q1dnfc = True
q1ua = False
q2 = True
q2dnfc = True
q2ua = True



for i in range(1,25):
    #clean-up
    subprocess.check_output(['rm','-f','customer.tbl','lineitem.tbl','nation.tbl','orders.tbl','partsupp.tbl','part.tbl','region.tbl','supplier.tbl'])
    subprocess.check_output(['rm','-f','customer.tbl2','lineitem.tbl2','nation.tbl2','orders.tbl2','partsupp.tbl2','lineitem.tbl2','region.tbl2','supplier.tbl2'])
    subprocess.check_output(['rm','-f','lineitem.tbl2m','part.tbl2m','supplier.tbl2m'])
    #create
    subprocess.check_output(['./dbgen','-f','-s'+str(i*0.5)])
    subprocess.check_output(['chmod','777','customer.tbl','lineitem.tbl','nation.tbl','orders.tbl','partsupp.tbl','part.tbl','region.tbl','supplier.tbl'])
    subprocess.check_output(['python','sc.py','customer.tbl','lineitem.tbl','nation.tbl','orders.tbl','partsupp.tbl','part.tbl','region.tbl','supplier.tbl'])
    subprocess.check_output(['python','line.py'])
    subprocess.check_output(['python','part.py'])
    subprocess.check_output(['python','supp.py'])
    #create populate
    #Evaluate normal query
    subprocess.check_output(['psql','-Uetienne','-dpostgres','-a','-fcreate.sql'])

    print i

    if q1:
        timestamp = int(time.time()*1000)
        subprocess.check_output(['psql','-Uetienne','-dpostgres','-a','-fq1.sql'])
        w1.write(str(int(time.time()*1000)-timestamp)+',')
        if int(time.time()*1000)-timestamp >= 580000:
            q1 = False

    if q1dnfc:
        timestamp = int(time.time()*1000)
        subprocess.check_output(['psql','-Uetienne','-dpostgres','-a','-fq1dnfc.sql'])
        w3.write(str(int(time.time()*1000)-timestamp)+',')
        if int(time.time()*1000)-timestamp >= 580000:
            q1dnfc = False

    if q1ua:
        timestamp = int(time.time()*1000)
        subprocess.check_output(['psql','-Uetienne','-dpostgres','-a','-fq1ua.sql'])
        w2.write(str(int(time.time()*1000)-timestamp)+',')
        if int(time.time()*1000)-timestamp >= 580000:
            q1ua = False

    if q2:
        timestamp = int(time.time()*1000)
        subprocess.check_output(['psql','-Uetienne','-dpostgres','-a','-fq2.sql'])
        w4.write(str(int(time.time()*1000)-timestamp)+',')
        if int(time.time()*1000)-timestamp >= 580000:
            q2 = False

    if q2dnfc:
        timestamp = int(time.time()*1000)
        subprocess.check_output(['psql','-Uetienne','-dpostgres','-a','-fq2dnfc.sql'])
        w6.write(str(int(time.time()*1000)-timestamp)+',')
        if int(time.time()*1000)-timestamp >= 580000:
            q2dnfc = False

    if q2ua:
        timestamp = int(time.time()*1000)
        subprocess.check_output(['psql','-Uetienne','-dpostgres','-a','-fq2ua.sql'])
        w5.write(str(int(time.time()*1000)-timestamp)+',')
        if int(time.time()*1000)-timestamp >= 580000:
            q2ua = False

w1.close()
w2.close()
w3.close()
w4.close()
w5.close()
w6.close()
