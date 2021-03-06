
$max = 100;

sub check {
  my $status = shift;
  if ($status != 0) {
    print "system() call returned $status\n";
    exit -1;
  }
}

sub simple_test {
  $cmprsn = shift;
  for ($i = 0; $i <= 7; $i++) {
    unlink("recovery.db");
    unlink("recovery.db.jrn0");
    unlink("recovery.db.jrn1");

    print "===========================================================\n";
    print "inserting $max keys...\n";
    for ($k = 0; $k < $max; $k++) {
      #`cp recovery.db rec-$k.db`;
      #`cp recovery.db.jrn0 rec-$k.db.jrn0`;
      #`cp recovery.db.jrn1 rec-$k.db.jrn1`;
      check(system("./recovery insert 64 8 $k 0 $cmprsn $i"));
      check(system("./recovery recover $cmprsn"));
      check(system("./recovery verify 64 8 $k 0 $cmprsn 1"));
    }

    print "erasing $max keys...\n";
    for ($k = $max - 1; $k >= 0; $k--) {
      #`cp recovery.db rec-$k.db`;
      #`cp recovery.db.jrn0 rec-$k.db.jrn0`;
      #`cp recovery.db.jrn1 rec-$k.db.jrn1`;
      check(system("./recovery erase 64 $k 0 $cmprsn $i"));
      check(system("./recovery recover $cmprsn"));
      check(system("./recovery verify 64 8 $k 0 $cmprsn 0"));
    }
  }
}

sub extended_test {
  $cmprsn = shift;
  for ($i = 0; $i <= 10; $i++) {
    unlink("recovery.db");
    unlink("recovery.db.jrn0");
    unlink("recovery.db.jrn1");

    print "===========================================================\n";
    print "inserting $max keys...\n";
    for ($k = 0; $k < $max; $k++) {
      #`cp recovery.db rec-$k.db`;
      #`cp recovery.db.jrn0 rec-$k.db.jrn0`;
      #`cp recovery.db.jrn1 rec-$k.db.jrn1`;
      check(system("./recovery insert 1024 1024 $k 0 $cmprsn $i"));
      check(system("./recovery recover $cmprsn"));
      check(system("./recovery verify 1024 1024 $k 0 $cmprsn 1"));
    }

    print "erasing $max keys...\n";
    for ($k = $max - 1; $k >= 0; $k--) {
      #`cp recovery.db rec-$k.db`;
      #`cp recovery.db.jrn0 rec-$k.db.jrn0`;
      #`cp recovery.db.jrn1 rec-$k.db.jrn1`;
      check(system("./recovery erase 1024 $k 0 $cmprsn $i"));
      check(system("./recovery recover $cmprsn"));
      check(system("./recovery verify 1024 1024 $k 0 $cmprsn 0"));
    }
  }
}

sub duplicate_test {
  $cmprsn = shift;
  for ($i = 1; $i <= 7; $i++) {
    unlink("recovery.db");
    unlink("recovery.db.jrn0");
    unlink("recovery.db.jrn1");

    print "===========================================================\n";
    print "inserting $max keys...\n";
    for ($k = 0; $k < $max; $k++) {
      #`cp recovery.db rec-$k.db`;
      #`cp recovery.db.jrn0 rec-$k.db.jrn0`;
      #`cp recovery.db.jrn1 rec-$k.db.jrn1`;
      check(system("./recovery insert 8 8 $k 1 $cmprsn $i"));
      check(system("./recovery recover $cmprsn"));
      check(system("./recovery verify 8 8 $k 1 $cmprsn 1"));
    }

    print "erasing $max keys...\n";
    for ($k = $max - 1; $k >= 0; $k--) {
      check(system("./recovery erase 8 $k 1 $cmprsn $i"));
      check(system("./recovery recover $cmprsn"));
      check(system("./recovery verify 8 8 $k 1 $cmprsn 0"));
    }
  }
}

sub extended_duplicate_test {
  $cmprsn = shift;
  for ($i = 1; $i <= 10; $i++) {
    unlink("recovery.db");
    unlink("recovery.db.jrn0");
    unlink("recovery.db.jrn1");

    print "inserting $max keys...\n";
    for ($k = 0; $k < $max; $k++) {
      #`cp recovery.db rec-$k.db`;
      #`cp recovery.db.jrn0 rec-$k.db.jrn0`;
      #`cp recovery.db.jrn1 rec-$k.db.jrn1`;
      check(system("./recovery insert 1024 1024 $k 1 $cmprsn $i"));
      check(system("./recovery recover $cmprsn"));
      check(system("./recovery verify 1024 1024 $k 1 $cmprsn 1"));
    }

    print "erasing $max keys...\n";
    for ($k = $max - 1; $k >= 0; $k--) {
      #`cp recovery.db rec-$k.db`;
      #`cp recovery.db.jrn0 rec-$k.db.jrn0`;
      #`cp recovery.db.jrn1 rec-$k.db.jrn1`;
      check(system("./recovery erase 1024 $k 1 $cmprsn $i"));
      check(system("./recovery recover $cmprsn"));
      check(system("./recovery verify 1024 1024 $k 1 $cmprsn 0"));
    }
  }
}

print "----------------------------\nsimple_test\n";
simple_test(0);

print "----------------------------\nextended_test\n";
extended_test(0);

print "----------------------------\nduplicate_test\n";
duplicate_test(0);

print "----------------------------\nextended_duplicate_test\n";
extended_duplicate_test(0);

print "\nsuccess!\n";
exit(0);
