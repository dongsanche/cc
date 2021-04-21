import os

# k = 0
# testnum = 6
# runtimes = 20
# steplen = 1

# c = 0
# for i in range(testnum):
#     for j in range(runtimes):
#         ofile = open("scratch/conf.txt",'r')
#         idx = int(ofile.readline().split('=')[1])
#         ofile.close()
#         if c >= idx:
#             sfile = open("scratch/conf.txt",'w')
#             sfile.write("idx=" + str(idx) + "\r\nvnum=600\r\nrange=250\r\nseghop="+str(k)+"\r\nenable-etar=true\r\nCarryTimeThreshold=20")
#             sfile.close()

#             os.system("./waf --run scratch/MyVanet")

#             sfile = open("scratch/conf.txt",'w')
#             sfile.write("idx=" + str(idx + 1) + "\r\nvnum=600\r\nrange=250\r\nseghop="+str(k)+"\r\nenable-etar=true\r\nCarryTimeThreshold=20")
#             sfile.close()

#         c += 1
#     k += steplen

# sfile = open("scratch/conf.txt",'w')
# sfile.write("idx=0\r\nvnum=600\r\nrange=250\r\nseghop=2\r\nenable-etar=true\r\nCarryTimeThreshold=20")
# sfile.close()

k = 500
testnum = 12
runtimes = 10
steplen = 500

c = 0
for i in range(testnum):
    for j in range(runtimes):
        ofile = open("scratch/conf.txt",'r')
        idx = int(ofile.readline().split('=')[1])
        ofile.close()
        if c >= idx:
            sfile = open("scratch/conf.txt",'w')
            sfile.write("idx=" + str(idx) + "\r\nvnum=600\r\nrange=250\r\nseghop=2\r\nenable-etar=false\r\nCarryTimeThreshold=20\r\nDistanceRange=" + str(k))
            sfile.close()

            os.system("./waf --run scratch/MyVanet")

            sfile = open("scratch/conf.txt",'w')
            sfile.write("idx=" + str(idx + 1) + "\r\nvnum=600\r\nrange=250\r\nseghop=2\r\nenable-etar=false\r\nCarryTimeThreshold=20\r\nDistanceRange=" +str(k))
            sfile.close()

        c += 1
    k += steplen

sfile = open("scratch/conf.txt",'w')
sfile.write("idx=0\r\nvnum=600\r\nrange=250\r\nseghop=2\r\nenable-etar=false\r\nCarryTimeThreshold=20\r\nDistanceRange=1000")
sfile.close()



# for i in range(10):
#     os.system("./waf --run scratch/MyVanet")