/*
 * Copyright (C) 2005-2014 Christoph Rupp (chris@crupp.de).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

using Hamster;
using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Unittests
{
    [TestClass()]
    [DeploymentItem("..\\win32\\msvc2008\\out\\dll_debug\\hamsterdb-2.1.7.dll")]
    public class EnvironmentTest
    {
        void checkEqual(byte[] lhs, byte[] rhs)
        {
            Assert.AreEqual(lhs.Length, rhs.Length);
            for (int i = 0; i < lhs.Length; i++)
                Assert.AreEqual(lhs[i], rhs[i]);
        }

        [TestMethod()]
        public void CreateString() {
            Hamster.Environment env = new Hamster.Environment();
            try {
                env.Create("ntest.db");
                env.Close();
            }
            catch (DatabaseException e) {
                Assert.Fail("unexpected exception " + e);
            }
        }

        [TestMethod()]
        public void CreateStringNull() {
            Hamster.Environment env = new Hamster.Environment();
            try {
                env.Create(null);
                env.Close();
            }
            catch (DatabaseException e) {
                Assert.AreEqual(HamConst.HAM_INV_PARAMETER, e.ErrorCode);
            }
        }

        [TestMethod()]
        public void CreateStringInt() {
            Hamster.Environment env = new Hamster.Environment();
            try {
                env.Create(null, HamConst.HAM_IN_MEMORY);
                env.Close();
            }
            catch (DatabaseException e) {
                Assert.Fail("unexpected exception " + e);
            }
        }

        [TestMethod()]
        public void CreateStringIntInt() {
            Hamster.Environment env = new Hamster.Environment();
            try {
                env.Create("ntest.db", 0, 0644);
                env.Close();
            }
            catch (DatabaseException e) {
                Assert.Fail("unexpected exception " + e);
            }
        }

        [TestMethod()]
        public void CreateStringIntIntParameter() {
            Hamster.Environment env = new Hamster.Environment();
            Parameter[] param = new Parameter[1];
            param[0] = new Parameter();
            param[0].name = HamConst.HAM_PARAM_MAX_DATABASES;
            param[0].value = 10;
            try {
                env.Create("ntest.db", 0, 0644, param);
                env.Close();
            }
            catch (DatabaseException e) {
                Assert.Fail("Unexpected exception " + e);
            }
        }

        [TestMethod()]
        public void CreateStringIntIntParameterNeg() {
            Hamster.Environment env = new Hamster.Environment();
            Parameter[] param = new Parameter[1];
            param[0] = new Parameter();
            param[0].name = HamConst.HAM_PARAM_PAGESIZE;
            param[0].value = 777;
            try {
                env.Create("ntest.db", 0, 0644, param);
                env.Close();
            }
            catch (DatabaseException e) {
                Assert.AreEqual(HamConst.HAM_INV_PAGESIZE, e.ErrorCode);
            }
        }

        [TestMethod()]
        public void OpenString() {
            Hamster.Environment env = new Hamster.Environment();
            try {
                env.Create("ntest.db");
                env.Close();
                env.Open("ntest.db");
                env.Close();
            }
            catch (DatabaseException e) {
                Assert.Fail("unexpected exception " + e);
            }
        }

        [TestMethod()]
        public void OpenStringNegative() {
            Hamster.Environment env = new Hamster.Environment();
            try {
                env.Open("ntestxxxxx.db");
                env.Close();
            }
            catch (DatabaseException e) {
                Assert.AreEqual(HamConst.HAM_FILE_NOT_FOUND, e.ErrorCode);
            }
        }

        [TestMethod()]
        public void OpenStringIntIntParameter() {
            Hamster.Environment env = new Hamster.Environment();
            Parameter[] param = new Parameter[1];
            param[0] = new Parameter();
            param[0].name = HamConst.HAM_PARAM_CACHESIZE;
            param[0].value = 1024;
            try {
                env.Create("ntest.db", 0, 0644, param);
                env.Close();
                env.Open("ntest.db", 0, param);
                env.Close();
            }
            catch (DatabaseException e) {
                Assert.Fail("unexpected exception " + e);
            }
        }

        [TestMethod()]
        public void CreateDatabaseShort() {
            Hamster.Environment env = new Hamster.Environment();
            byte[] k=new byte[5];
            byte[] r=new byte[5];
            try {
                env.Create("ntest.db");
                Database db = env.CreateDatabase((short)13);
                db.Insert(k, r);
                db.Close();
                db = env.OpenDatabase((short)13);
                byte[] f = db.Find(k);
                checkEqual(r, f);
                // db.Close();
                env.Close();
            }
            catch (DatabaseException e) {
                Assert.Fail("unexpected exception " + e);
            }
        }

        [TestMethod()]
        public void CreateDatabaseNegative() {
            Hamster.Environment env = new Hamster.Environment();
            try {
                env.Create("ntest.db");
                Database db = env.CreateDatabase((short)0);
            }
            catch (DatabaseException e) {
                Assert.AreEqual(HamConst.HAM_INV_PARAMETER, e.ErrorCode);
            }
            env.Close();
        }

        [TestMethod()]
        public void OpenDatabaseNegative() {
            Hamster.Environment env = new Hamster.Environment();
            try {
                env.Create("ntest.db");
                Database db = env.OpenDatabase((short)99);
            }
            catch (DatabaseException e) {
                Assert.AreEqual(HamConst.HAM_DATABASE_NOT_FOUND, e.ErrorCode);
            }
            env.Close();
        }

        [TestMethod()]
        public void RenameDatabase() {
            Hamster.Environment env = new Hamster.Environment();
            byte[] k = new byte[5];
            byte[] r = new byte[5];
            try {
                env.Create("ntest.db");
                Database db = env.CreateDatabase((short)13);
                db.Insert(k, r);
                db.Close();
                env.RenameDatabase((short)13, (short)15);
                db = env.OpenDatabase((short)15);
                byte[] f = db.Find(k);
                checkEqual(r, f);
                // db.Close();
                env.Close();
            }
            catch (DatabaseException e) {
                Assert.Fail("unexpected exception " + e);
            }
        }

        [TestMethod()]
        public void EraseDatabase() {
            Hamster.Environment env = new Hamster.Environment();
            byte[] k = new byte[5];
            byte[] r = new byte[5];

            env.Create("ntest.db");
            Database db = env.CreateDatabase((short)13);
            db.Insert(k, r);
            db.Close();
            env.EraseDatabase((short)13);
            try {
                db = env.OpenDatabase((short)15);
            }
            catch (DatabaseException e) {
                Assert.AreEqual(HamConst.HAM_DATABASE_NOT_FOUND, e.ErrorCode);
            }
            env.Close();
        }

        [TestMethod()]
        public void EraseUnknownDatabase() {
            Hamster.Environment env = new Hamster.Environment();
            byte[] k = new byte[5];
            byte[] r = new byte[5];

            env.Create("ntest.db");
            Database db = env.CreateDatabase((short)13);
            db.Insert(k, r);
            db.Close();
            try {
                env.EraseDatabase((short)99);
            }
            catch (DatabaseException e) {
                Assert.AreEqual(HamConst.HAM_DATABASE_NOT_FOUND, e.ErrorCode);
            }
            env.Close();
        }

        [TestMethod()]
        public void Flush()
        {
            Hamster.Environment env = new Hamster.Environment();
            env.Create("ntest.db");
            env.Flush();
            env.Close();
        }

        [TestMethod()]
        public void GetDatabaseNames() {
            Database db;
            short[] names;
            short[] s1 ={ 13 };
            short[] s2 ={ 13, 14 };
            short[] s3 ={ 13, 14, 15 };
            Hamster.Environment env = new Hamster.Environment();

            env.Create("ntest.db");
            db = env.CreateDatabase(13);
            names = env.GetDatabaseNames();
            Assert.AreEqual(s1.Length, names.Length);
            for (int i = 0; i < s1.Length; i++)
                Assert.AreEqual(s1[i], names[i]);

            db = env.CreateDatabase(14);
            names = env.GetDatabaseNames();
            Assert.AreEqual(s2.Length, names.Length);
            for (int i = 0; i < s2.Length; i++)
                Assert.AreEqual(s2[i], names[i]);

            db = env.CreateDatabase(15);
            names = env.GetDatabaseNames();
            Assert.AreEqual(s3.Length, names.Length);
            for (int i = 0; i < s3.Length; i++)
                Assert.AreEqual(s3[i], names[i]);

            env.Close();
        }

    }
}
